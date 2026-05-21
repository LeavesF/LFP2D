// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/AI/LFPEnemyBehaviorData.h"
#include "LFP2D/Card/LFPCardDataAsset.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Kismet/GameplayStatics.h"

namespace
{
ULFPGameInstance* GetLFPGameInstance(const UObject* WorldContext)
{
    if (!WorldContext)
    {
        return nullptr;
    }

    UWorld* World = WorldContext->GetWorld();
    return World ? Cast<ULFPGameInstance>(World->GetGameInstance()) : nullptr;
}
}

FEnemyActionPlan FEnemySkillPlanCandidate::ToActionPlan() const
{
    FEnemyActionPlan Plan;
    if (!bIsValid)
    {
        return Plan;
    }

    Plan.EnemyUnit = EnemyUnit;
    Plan.PlannedSkill = Skill;
    Plan.TargetUnit = TargetUnit;
    Plan.TargetTile = TargetTile;
    Plan.CasterPositionTile = CasterPositionTile;
    Plan.EffectAreaTiles = EffectAreaTiles;
    Plan.bIsValid = true;
    return Plan;
}

ALFPAIController::ALFPAIController()
{
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    ControlledUnit = nullptr;
    GridManager = nullptr;
}

void ALFPAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledUnit = Cast<ALFPTacticsUnit>(InPawn);
    if (ControlledUnit)
    {
        if (BehaviorTree && BlackboardComponent)
        {
            BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
        }

        TArray<AActor*> GridManagers;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), GridManagers);
        if (GridManagers.Num() > 0)
        {
            GridManager = Cast<ALFPHexGridManager>(GridManagers[0]);
        }

        RunBehaviorTree(BehaviorTree);
    }
}

void ALFPAIController::OnUnPossess()
{
    Super::OnUnPossess();
}

void ALFPAIController::StartUnitTurn()
{
    if (!ControlledUnit || !BlackboardComponent) return;

    BlackboardComponent->SetValueAsObject("TargetUnit", nullptr);
    BlackboardComponent->SetValueAsObject("TargetTile", nullptr);
    BlackboardComponent->SetValueAsBool("IsInAttackRange", false);
    BlackboardComponent->SetValueAsBool("CanAttack", false);
    BlackboardComponent->SetValueAsBool("ShouldEndTurn", false);

    ALFPTacticsUnit* BestTarget = FindBestTarget();
    BlackboardComponent->SetValueAsObject("TargetUnit", BestTarget);
}

void ALFPAIController::EndUnitTurn()
{
}

ALFPTacticsUnit* ALFPAIController::FindBestTarget() const
{
    return FindBestHatredTarget();
}

ALFPTacticsUnit* ALFPAIController::FindBestHatredTarget() const
{
    return FindBestHatredTarget(nullptr);
}

ALFPTacticsUnit* ALFPAIController::FindBestHatredTarget(const TMap<ALFPTacticsUnit*, int32>* ExistingTargetLocks) const
{
    if (!ControlledUnit || !GridManager) return nullptr;

    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), AllUnits);

    ALFPTacticsUnit* BestTarget = nullptr;
    float BestHatredValue = -MAX_FLT;

    for (AActor* Actor : AllUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (Unit && Unit->IsAlive() && Unit->IsAlly())
        {
            const float HatredValue = CalculateTargetHatredValue(Unit, ExistingTargetLocks);
            if (HatredValue > BestHatredValue)
            {
                BestHatredValue = HatredValue;
                BestTarget = Unit;
            }
        }
    }

    return BestTarget;
}

ALFPTacticsUnit* ALFPAIController::FindBestSkillTarget(ULFPSkillBase* Skill) const
{
    FEnemySkillPlanCandidate Candidate;
    return BuildBestSkillPlanCandidate(Skill, 0, Candidate) ? Candidate.TargetUnit : nullptr;
}

ALFPHexTile* ALFPAIController::FindBestMovementTile(ALFPTacticsUnit* Target) const
{
    if (!ControlledUnit || !Target || !GridManager) return nullptr;

    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(ControlledUnit->GetCurrentTile(), ControlledUnit->GetCurrentMovePoints(), ControlledUnit->GetAffiliation());

    ALFPHexTile* BestTile = nullptr;
    float BestPositionValue = -MAX_FLT;

    for (ALFPHexTile* Tile : MovementRange)
    {
        if (Tile->GetUnitOnTile()) continue;

        float PositionValue = CalculatePositionValue(Tile, Target);
        if (PositionValue > BestPositionValue)
        {
            BestPositionValue = PositionValue;
            BestTile = Tile;
        }
    }

    return BestTile;
}

float ALFPAIController::CalculateThreatValue(ALFPTacticsUnit* Target) const
{
    if (!ControlledUnit || !Target) return 0.0f;

    float ThreatValue = Target->GetCurrentAttack() * (1.0f - (float)Target->GetCurrentHealth() / Target->GetMaxHealth());

    const int32 Distance = FLFPHexCoordinates::Distance(
        ControlledUnit->GetCurrentCoordinates(),
        Target->GetCurrentCoordinates());
    const float DistanceFactor = 1.0f / FMath::Max(Distance, 1);

    if (BehaviorData)
    {
        if (BehaviorData->bPrioritizeWeakTargets)
        {
            const float HealthRatio = (float)Target->GetCurrentHealth() / Target->GetMaxHealth();
            ThreatValue *= (2.0f - HealthRatio);
        }

        ThreatValue *= BehaviorData->Aggressiveness;
    }

    return ThreatValue * DistanceFactor;
}

float ALFPAIController::CalculateTargetHatredValue(ALFPTacticsUnit* Target) const
{
    return CalculateTargetHatredValue(Target, nullptr);
}

float ALFPAIController::CalculateTargetHatredValue(
    ALFPTacticsUnit* Target,
    const TMap<ALFPTacticsUnit*, int32>* ExistingTargetLocks) const
{
    if (!ControlledUnit || !Target)
    {
        return 0.0f;
    }

    float TagHatred = 0.0f;
    if (BehaviorData)
    {
        const FGameplayTagContainer& TargetTags = Target->GetSpecialTags();
        for (const TPair<FGameplayTag, float>& Pair : BehaviorData->TargetTagHatredModifiers)
        {
            if (Pair.Key.IsValid() && TargetTags.HasTag(Pair.Key))
            {
                TagHatred += Pair.Value;
            }
        }
    }

    const int32 Distance = FLFPHexCoordinates::Distance(
        ControlledUnit->GetCurrentCoordinates(),
        Target->GetCurrentCoordinates());
    const float DistanceHatred = 100.0f / FMath::Max(Distance, 1);
    const float ThreatHatred = static_cast<float>(Target->GetCurrentAttack());

    const float TagWeight = BehaviorData ? BehaviorData->TagHatredWeight : 1.0f;
    const float DistanceWeight = BehaviorData ? BehaviorData->DistanceHatredWeight : 1.0f;
    const float ThreatWeight = BehaviorData ? BehaviorData->ThreatHatredWeight : 1.0f;
    const float ExistingLockWeight = BehaviorData ? BehaviorData->ExistingTargetLockHatredWeight : 1.0f;
    const float ExistingLockPenalty = BehaviorData ? BehaviorData->ExistingTargetLockHatredPenalty : 50.0f;
    const int32* ExistingLockCountPtr = ExistingTargetLocks ? ExistingTargetLocks->Find(Target) : nullptr;
    const int32 ExistingLockCount = ExistingLockCountPtr ? *ExistingLockCountPtr : 0;

    return
        (TagWeight * TagHatred) +
        (DistanceWeight * DistanceHatred) +
        (ThreatWeight * ThreatHatred) -
        (ExistingLockWeight * ExistingLockPenalty * ExistingLockCount);
}

float ALFPAIController::CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target) const
{
    if (!Tile || !Target) return 0.0f;

    float PositionValue = 0.0f;

    const int32 DistanceToTarget = FLFPHexCoordinates::Distance(
        Tile->GetCoordinates(),
        Target->GetCurrentCoordinates());
    PositionValue += 10.0f / FMath::Max(DistanceToTarget, 1);

    if (DistanceToTarget <= ControlledUnit->GetAttackRange())
    {
        PositionValue += 20.0f;
    }

    TArray<AActor*> EnemyUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), EnemyUnits);

    for (AActor* Actor : EnemyUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (Unit && Unit != ControlledUnit && Unit->IsEnemy() && Unit->IsAlive())
        {
            const int32 DistanceToAlly = FLFPHexCoordinates::Distance(
                Tile->GetCoordinates(),
                Unit->GetCurrentCoordinates());

            if (DistanceToAlly <= 2)
            {
                PositionValue += 5.0f / FMath::Max(DistanceToAlly, 1);
            }
        }
    }

    return PositionValue;
}

FEnemyActionPlan ALFPAIController::CreateActionPlan(ULFPSkillBase* PreAllocatedSkill)
{
    if (PreAllocatedSkill)
    {
        FEnemyActionPlan Plan;
        Plan.EnemyUnit = ControlledUnit;

        if (!ControlledUnit || !GridManager)
        {
            return Plan;
        }

        FEnemySkillPlanCandidate Candidate;
        if (BuildBestSkillPlanCandidate(PreAllocatedSkill, 0, Candidate))
        {
            return Candidate.ToActionPlan();
        }

        return CreateActionPlanWithTargetLocks(nullptr);
    }

    return CreateActionPlanWithTargetLocks(nullptr);
}

FEnemyActionPlan ALFPAIController::CreateActionPlanWithTargetLocks(
    const TMap<ALFPTacticsUnit*, int32>* ExistingTargetLocks)
{
    FEnemyActionPlan Plan;
    Plan.EnemyUnit = ControlledUnit;

    if (!ControlledUnit || !GridManager)
    {
        return Plan;
    }

    ALFPTacticsUnit* TargetUnit = FindBestHatredTarget(ExistingTargetLocks);
    if (!TargetUnit)
    {
        return CreateMovementOnlyPlan(nullptr);
    }

    BuildEnemyCardSkills();

    ULFPSkillBase* SelectedSkill = nullptr;
    ALFPHexTile* CasterTile = nullptr;
    TArray<ALFPHexTile*> EffectAreaTiles;
    if (SelectWeightedSkillForTarget(TargetUnit, SelectedSkill, CasterTile, EffectAreaTiles))
    {
        Plan.PlannedSkill = SelectedSkill;
        Plan.TargetUnit = TargetUnit;
        Plan.TargetTile = TargetUnit->GetCurrentTile();
        Plan.CasterPositionTile = CasterTile;
        Plan.EffectAreaTiles = EffectAreaTiles;
        Plan.bIsValid = Plan.TargetTile != nullptr && Plan.CasterPositionTile != nullptr;
        return Plan;
    }

    FEnemySkillPlanCandidate MovementCandidate;
    MovementCandidate.TargetUnit = TargetUnit;
    MovementCandidate.TargetTile = TargetUnit->GetCurrentTile();
    return CreateMovementOnlyPlan(&MovementCandidate);
}

ULFPSkillBase* ALFPAIController::SelectBestSkill()
{
    if (!ControlledUnit)
    {
        return nullptr;
    }

    TArray<FEnemySkillPlanCandidate> Candidates;
    TArray<ULFPSkillBase*> Skills = ControlledUnit->GetAvailableSkills();
    for (ULFPSkillBase* Skill : Skills)
    {
        FEnemySkillPlanCandidate Candidate;
        if (BuildBestSkillPlanCandidate(Skill, 0, Candidate))
        {
            Candidates.Add(Candidate);
        }
    }

    NormalizeCandidateScores(Candidates);

    FEnemySkillPlanCandidate* BestCandidate = nullptr;
    for (FEnemySkillPlanCandidate& Candidate : Candidates)
    {
        if (!BestCandidate ||
            Candidate.TotalScore > BestCandidate->TotalScore ||
            (FMath::IsNearlyEqual(Candidate.TotalScore, BestCandidate->TotalScore) &&
                Candidate.EffectivePriority > BestCandidate->EffectivePriority) ||
            (FMath::IsNearlyEqual(Candidate.TotalScore, BestCandidate->TotalScore) &&
                FMath::IsNearlyEqual(Candidate.EffectivePriority, BestCandidate->EffectivePriority) &&
                Candidate.HatredValue > BestCandidate->HatredValue))
        {
            BestCandidate = &Candidate;
        }
    }

    return BestCandidate ? BestCandidate->Skill : ControlledUnit->GetDefaultAttackSkill();
}

ALFPHexTile* ALFPAIController::FindBestCasterPosition(ULFPSkillBase* Skill, ALFPHexTile* TargetTile)
{
    float BestValue = -MAX_FLT;
    return FindBestCasterPositionInternal(Skill, TargetTile, BestValue);
}

void ALFPAIController::BuildEnemyCardSkills()
{
    RuntimeEnemyCardSkills.Empty();

    if (!ControlledUnit)
    {
        return;
    }

    const TSoftObjectPtr<ULFPCardDataAsset> AttackCard = GetGlobalAttackCardForUnit();
    if (!AttackCard.IsNull())
    {
        AddEnemyCardSkillFromCardData(AttackCard);
    }

    ULFPGameInstance* GameInstance = GetLFPGameInstance(this);
    if (!GameInstance || !GameInstance->UnitRegistry)
    {
        return;
    }

    FLFPUnitRegistryEntry Entry;
    if (!GameInstance->UnitRegistry->FindEntry(ControlledUnit->UnitTypeID, Entry))
    {
        return;
    }

    for (const TSoftObjectPtr<ULFPCardDataAsset>& CardData : Entry.DefaultCarriedCards)
    {
        AddEnemyCardSkillFromCardData(CardData);
    }
}

bool ALFPAIController::AddEnemyCardSkillFromCardData(const TSoftObjectPtr<ULFPCardDataAsset>& CardData)
{
    if (CardData.IsNull() || !ControlledUnit)
    {
        return false;
    }

    ULFPCardDataAsset* LoadedCardData = CardData.LoadSynchronous();
    if (!LoadedCardData || !LoadedCardData->IsValidCardData())
    {
        return false;
    }

    ULFPSkillBase* RuntimeSkill = NewObject<ULFPSkillBase>(this, LoadedCardData->SkillClass);
    if (!RuntimeSkill)
    {
        return false;
    }

    RuntimeSkill->InitSkill(ControlledUnit);
    RuntimeSkill->SkillName = LoadedCardData->DisplayName;
    RuntimeSkill->SkillDescription = LoadedCardData->Description;
    RuntimeSkill->SkillIcon = LoadedCardData->Icon;
    if (LoadedCardData->ActionPointCost != INDEX_NONE)
    {
        RuntimeSkill->ActionPointCost = LoadedCardData->ActionPointCost;
    }

    RuntimeEnemyCardSkills.Add(RuntimeSkill);
    return true;
}

TSoftObjectPtr<ULFPCardDataAsset> ALFPAIController::GetGlobalAttackCardForUnit() const
{
    const ULFPGameInstance* GameInstance = GetLFPGameInstance(this);
    if (!GameInstance || !ControlledUnit)
    {
        return TSoftObjectPtr<ULFPCardDataAsset>();
    }

    const FGameplayTag AttackTag = FindFirstControlledUnitTagWithPrefix(TEXT("Unit.Attack."));
    if (!AttackTag.IsValid())
    {
        return TSoftObjectPtr<ULFPCardDataAsset>();
    }

    const FString TagName = AttackTag.GetTagName().ToString();
    if (TagName.EndsWith(TEXT(".Melee")))
    {
        return GameInstance->FallbackMeleeAttackCard;
    }
    if (TagName.EndsWith(TEXT(".Ranged")))
    {
        return GameInstance->FallbackRangedAttackCard;
    }
    if (TagName.EndsWith(TEXT(".Magic")))
    {
        return GameInstance->FallbackMagicAttackCard;
    }

    return TSoftObjectPtr<ULFPCardDataAsset>();
}

FGameplayTag ALFPAIController::FindFirstControlledUnitTagWithPrefix(const FString& Prefix) const
{
    if (!ControlledUnit)
    {
        return FGameplayTag();
    }

    const FGameplayTagContainer& UnitTags = ControlledUnit->GetSpecialTags();
    for (const FGameplayTag& Tag : UnitTags)
    {
        if (Tag.GetTagName().ToString().StartsWith(Prefix))
        {
            return Tag;
        }
    }

    return FGameplayTag();
}

bool ALFPAIController::IsEnemyTargetSkill(ULFPSkillBase* Skill) const
{
    if (!Skill || Skill->IsPassiveSkill() || Skill->CurrentCooldown > 0 || Skill->BasePriority <= 0.0f)
    {
        return false;
    }

    return
        Skill->TargetType == ESkillTargetType::SingleEnemy ||
        Skill->TargetType == ESkillTargetType::MutiEnemy ||
        Skill->TargetType == ESkillTargetType::AllEnemy ||
        Skill->TargetType == ESkillTargetType::SingleUnit ||
        Skill->TargetType == ESkillTargetType::MutiUnit ||
        Skill->TargetType == ESkillTargetType::AllUnit ||
        Skill->TargetType == ESkillTargetType::AnyTile;
}

bool ALFPAIController::SelectWeightedSkillForTarget(
    ALFPTacticsUnit* TargetUnit,
    ULFPSkillBase*& OutSkill,
    ALFPHexTile*& OutCasterTile,
    TArray<ALFPHexTile*>& OutEffectAreaTiles)
{
    OutSkill = nullptr;
    OutCasterTile = nullptr;
    OutEffectAreaTiles.Empty();

    if (!ControlledUnit || !TargetUnit)
    {
        return false;
    }

    ALFPHexTile* TargetTile = TargetUnit->GetCurrentTile();
    if (!TargetTile)
    {
        return false;
    }

    TArray<FEnemySkillPlanCandidate> Candidates;
    for (ULFPSkillBase* Skill : RuntimeEnemyCardSkills)
    {
        if (!IsEnemyTargetSkill(Skill))
        {
            continue;
        }

        float PositionValue = -MAX_FLT;
        FEnemySkillPlanCandidate Candidate;
        if (TryBuildCandidateForTarget(Skill, TargetUnit, TargetTile, 0, PositionValue, Candidate))
        {
            Candidate.EffectivePriority = Skill->BasePriority;
            Candidates.Add(Candidate);
        }
    }

    while (!Candidates.IsEmpty())
    {
        float TotalWeight = 0.0f;
        for (const FEnemySkillPlanCandidate& Candidate : Candidates)
        {
            TotalWeight += FMath::Max(0.0f, Candidate.Skill ? Candidate.Skill->BasePriority : 0.0f);
        }

        if (TotalWeight <= 0.0f)
        {
            return false;
        }

        float Roll = FMath::FRandRange(0.0f, TotalWeight);
        int32 ChosenIndex = INDEX_NONE;
        for (int32 Index = 0; Index < Candidates.Num(); ++Index)
        {
            Roll -= FMath::Max(0.0f, Candidates[Index].Skill ? Candidates[Index].Skill->BasePriority : 0.0f);
            if (Roll <= 0.0f)
            {
                ChosenIndex = Index;
                break;
            }
        }

        if (ChosenIndex == INDEX_NONE)
        {
            ChosenIndex = Candidates.Num() - 1;
        }

        const FEnemySkillPlanCandidate Candidate = Candidates[ChosenIndex];
        if (Candidate.bIsValid && Candidate.Skill && Candidate.CasterPositionTile)
        {
            OutSkill = Candidate.Skill;
            OutCasterTile = Candidate.CasterPositionTile;
            OutEffectAreaTiles = Candidate.EffectAreaTiles;
            return true;
        }

        Candidates.RemoveAt(ChosenIndex);
    }

    return false;
}

bool ALFPAIController::BuildBestSkillPlanCandidate(ULFPSkillBase* Skill, int32 PlanningOrderIndex, FEnemySkillPlanCandidate& OutCandidate) const
{
    OutCandidate = FEnemySkillPlanCandidate();

    if (!ControlledUnit || !GridManager || !Skill)
    {
        return false;
    }

    if (Skill->IsPassiveSkill() || Skill->CurrentCooldown > 0)
    {
        return false;
    }

    bool bFoundCandidate = false;
    FEnemySkillPlanCandidate BestCandidate;
    float BestHatred = -MAX_FLT;
    float BestPositionValue = -MAX_FLT;

    // 每个(敌人, 技能)只保留一个当前回合最优的完整候选，
    // 这里会把目标、施法位和效果范围一次性确定下来。
    if (Skill->TargetType == ESkillTargetType::Self)
    {
        ALFPHexTile* CurrentTile = ControlledUnit->GetCurrentTile();
        if (!CurrentTile)
        {
            return false;
        }

        // 自目标技能也允许先移动再释放，所以要把可达格一起纳入评估。
        TArray<ALFPHexTile*> SelfTargetTiles = GridManager->GetTilesInRange(CurrentTile, ControlledUnit->GetCurrentMovePoints(), ControlledUnit->GetAffiliation());
        SelfTargetTiles.AddUnique(CurrentTile);

        for (ALFPHexTile* SelfTargetTile : SelfTargetTiles)
        {
            float PositionValue = -MAX_FLT;
            FEnemySkillPlanCandidate Candidate;
            if (!TryBuildCandidateForTarget(Skill, ControlledUnit, SelfTargetTile, PlanningOrderIndex, PositionValue, Candidate))
            {
                continue;
            }

            if (!bFoundCandidate ||
                Candidate.HatredValue > BestHatred ||
                (FMath::IsNearlyEqual(Candidate.HatredValue, BestHatred) && PositionValue > BestPositionValue))
            {
                bFoundCandidate = true;
                BestHatred = Candidate.HatredValue;
                BestPositionValue = PositionValue;
                BestCandidate = Candidate;
            }
        }
    }
    else
    {
        TArray<ALFPTacticsUnit*> PotentialTargets;
        CollectPotentialTargets(Skill, PotentialTargets);

        for (ALFPTacticsUnit* TargetUnit : PotentialTargets)
        {
            if (!TargetUnit)
            {
                continue;
            }

            ALFPHexTile* TargetTile = TargetUnit->GetCurrentTile();
            if (!TargetTile)
            {
                continue;
            }

            float PositionValue = -MAX_FLT;
            FEnemySkillPlanCandidate Candidate;
            if (!TryBuildCandidateForTarget(Skill, TargetUnit, TargetTile, PlanningOrderIndex, PositionValue, Candidate))
            {
                continue;
            }

            if (!bFoundCandidate ||
                Candidate.HatredValue > BestHatred ||
                (FMath::IsNearlyEqual(Candidate.HatredValue, BestHatred) && PositionValue > BestPositionValue))
            {
                bFoundCandidate = true;
                BestHatred = Candidate.HatredValue;
                BestPositionValue = PositionValue;
                BestCandidate = Candidate;
            }
        }
    }

    if (!bFoundCandidate)
    {
        return false;
    }

    // 候选池里只保留该技能本回合最好的那个完整方案，避免排列组合膨胀。
    OutCandidate = BestCandidate;
    return true;
}

FEnemyActionPlan ALFPAIController::CreateMovementOnlyPlan(const FEnemySkillPlanCandidate* PreferredCandidate) const
{
    FEnemyActionPlan Plan;
    Plan.EnemyUnit = ControlledUnit;

    if (!ControlledUnit || !GridManager)
    {
        return Plan;
    }

    ALFPHexTile* MoveTargetTile = nullptr;
    if (PreferredCandidate)
    {
        // 优先朝“本回合其实成立，只是 AP 不够”的技能施法位铺路。
        Plan.TargetUnit = PreferredCandidate->TargetUnit;
        Plan.TargetTile = PreferredCandidate->TargetTile;
        MoveTargetTile = FindBestApproachTile(PreferredCandidate);
    }

    ALFPTacticsUnit* FallbackTarget = nullptr;
    if (!MoveTargetTile)
    {
        // 没有可铺路的高分候选时，退化为朝当前最高威胁目标逼近。
        FallbackTarget = (Plan.TargetUnit && Plan.TargetUnit->IsAlive()) ? Plan.TargetUnit : FindBestTarget();
        if (!Plan.TargetUnit)
        {
            Plan.TargetUnit = FallbackTarget;
        }
        if (!Plan.TargetTile && FallbackTarget)
        {
            Plan.TargetTile = FallbackTarget->GetCurrentTile();
        }
        if (FallbackTarget)
        {
            MoveTargetTile = FindBestMovementTile(FallbackTarget);
        }
    }

    if (!MoveTargetTile)
    {
        MoveTargetTile = ControlledUnit->GetCurrentTile();
    }

    Plan.CasterPositionTile = MoveTargetTile;
    Plan.bIsValid = MoveTargetTile != nullptr;
    return Plan;
}

void ALFPAIController::CollectPotentialTargets(ULFPSkillBase* Skill, TArray<ALFPTacticsUnit*>& OutTargets) const
{
    OutTargets.Empty();

    if (!ControlledUnit || !Skill)
    {
        return;
    }

    if (Skill->TargetType == ESkillTargetType::Self)
    {
        OutTargets.Add(ControlledUnit);
        return;
    }

    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), AllUnits);

    for (AActor* Actor : AllUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (!Unit || !Unit->IsAlive())
        {
            continue;
        }

        switch (Skill->TargetType)
        {
        case ESkillTargetType::SingleEnemy:
        case ESkillTargetType::MutiEnemy:
        case ESkillTargetType::AllEnemy:
        case ESkillTargetType::AnyTile:
            if (Skill->IsHostileTarget(Unit))
            {
                OutTargets.Add(Unit);
            }
            break;

        case ESkillTargetType::SingleAlly:
        case ESkillTargetType::MutiAlly:
        case ESkillTargetType::AllAlly:
            if (Unit->GetAffiliation() == ControlledUnit->GetAffiliation())
            {
                OutTargets.Add(Unit);
            }
            break;

        case ESkillTargetType::SingleUnit:
        case ESkillTargetType::MutiUnit:
        case ESkillTargetType::AllUnit:
            OutTargets.Add(Unit);
            break;

        default:
            break;
        }
    }
}

bool ALFPAIController::TryBuildCandidateForTarget(
    ULFPSkillBase* Skill,
    ALFPTacticsUnit* TargetUnit,
    ALFPHexTile* TargetTile,
    int32 PlanningOrderIndex,
    float& OutPositionValue,
    FEnemySkillPlanCandidate& OutCandidate) const
{
    OutPositionValue = -MAX_FLT;
    OutCandidate = FEnemySkillPlanCandidate();

    if (!ControlledUnit || !Skill || !TargetTile)
    {
        return false;
    }

    // 只有“除 AP 外其余条件都满足”的完整方案，才允许进入全局候选池。
    float PositionValue = -MAX_FLT;
    ALFPHexTile* CasterTile = FindBestCasterPositionInternal(Skill, TargetTile, PositionValue);
    if (!CasterTile)
    {
        return false;
    }

    if (!Skill->CanPlanFrom(CasterTile, TargetTile))
    {
        return false;
    }

    OutCandidate.EnemyUnit = ControlledUnit;
    OutCandidate.Skill = Skill;
    OutCandidate.TargetUnit = TargetUnit;
    OutCandidate.TargetTile = TargetTile;
    OutCandidate.CasterPositionTile = CasterTile;
    OutCandidate.APCost = FMath::Max(0, Skill->ActionPointCost);
    OutCandidate.EffectivePriority = Skill->GetEffectivePriority();
    OutCandidate.HatredValue = TargetUnit ? CalculateTargetHatredValue(TargetUnit) : 0.0f;
    OutCandidate.PlanningOrderIndex = PlanningOrderIndex;
    OutCandidate.bIsValid = true;
    BuildEffectAreaTiles(Skill, TargetTile, OutCandidate.EffectAreaTiles);

    OutPositionValue = PositionValue;
    return true;
}

void ALFPAIController::BuildEffectAreaTiles(ULFPSkillBase* Skill, ALFPHexTile* TargetTile, TArray<ALFPHexTile*>& OutTiles) const
{
    OutTiles.Empty();

    if (!Skill || !TargetTile || !GridManager)
    {
        return;
    }

    const FLFPHexCoordinates TargetCoords = TargetTile->GetCoordinates();
    for (const FLFPHexCoordinates& Offset : Skill->EffectRangeCoords)
    {
        const FLFPHexCoordinates AbsCoord(
            TargetCoords.Q + Offset.Q,
            TargetCoords.R + Offset.R);

        if (ALFPHexTile* EffectTile = GridManager->GetTileAtCoordinates(AbsCoord))
        {
            OutTiles.Add(EffectTile);
        }
    }

    if (OutTiles.Num() == 0)
    {
        OutTiles.Add(TargetTile);
    }
}

ALFPHexTile* ALFPAIController::FindBestCasterPositionInternal(ULFPSkillBase* Skill, ALFPHexTile* TargetTile, float& OutValue) const
{
    OutValue = -MAX_FLT;

    if (!ControlledUnit || !Skill || !TargetTile || !GridManager)
    {
        return nullptr;
    }

    ALFPHexTile* CurrentTile = ControlledUnit->GetCurrentTile();
    if (!CurrentTile)
    {
        return nullptr;
    }

    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(
        CurrentTile,
        ControlledUnit->GetCurrentMovePoints());
    MovementRange.AddUnique(CurrentTile);

    ALFPHexTile* BestTile = nullptr;
    float BestValue = -MAX_FLT;

    for (ALFPHexTile* Tile : MovementRange)
    {
        if (!Tile)
        {
            continue;
        }

        ALFPTacticsUnit* Occupant = Tile->GetUnitOnTile();
        if (Occupant && Occupant != ControlledUnit)
        {
            continue;
        }

        if (!Skill->CanPlanFrom(Tile, TargetTile))
        {
            continue;
        }

        float Value = 30.0f;
        const int32 MoveDistance = FLFPHexCoordinates::Distance(
            CurrentTile->GetCoordinates(),
            Tile->GetCoordinates());
        Value -= MoveDistance * 2.0f;

        if (Value > BestValue)
        {
            BestValue = Value;
            BestTile = Tile;
        }
    }

    OutValue = BestValue;
    return BestTile;
}

ALFPHexTile* ALFPAIController::FindBestApproachTile(const FEnemySkillPlanCandidate* PreferredCandidate) const
{
    if (!ControlledUnit || !GridManager)
    {
        return nullptr;
    }

    ALFPHexTile* CurrentTile = ControlledUnit->GetCurrentTile();
    if (!CurrentTile)
    {
        return nullptr;
    }

    ALFPHexTile* GoalTile = PreferredCandidate ? PreferredCandidate->CasterPositionTile : nullptr;
    if (!GoalTile)
    {
        return nullptr;
    }

    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(CurrentTile, ControlledUnit->GetCurrentMovePoints(), ControlledUnit->GetAffiliation());
    MovementRange.AddUnique(CurrentTile);

    ALFPHexTile* BestTile = nullptr;
    float BestScore = -MAX_FLT;

    for (ALFPHexTile* Tile : MovementRange)
    {
        if (!Tile)
        {
            continue;
        }

        ALFPTacticsUnit* Occupant = Tile->GetUnitOnTile();
        if (Occupant && Occupant != ControlledUnit)
        {
            continue;
        }

        const int32 GoalDistance = FLFPHexCoordinates::Distance(
            Tile->GetCoordinates(),
            GoalTile->GetCoordinates());
        const int32 MoveDistance = FLFPHexCoordinates::Distance(
            CurrentTile->GetCoordinates(),
            Tile->GetCoordinates());

        float Score = -static_cast<float>(GoalDistance * 100 + MoveDistance);
        if (PreferredCandidate->TargetTile)
        {
            const int32 TargetDistance = FLFPHexCoordinates::Distance(
                Tile->GetCoordinates(),
                PreferredCandidate->TargetTile->GetCoordinates());
            Score -= TargetDistance * 0.1f;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestTile = Tile;
        }
    }

    return BestTile;
}

ALFPTacticsUnit* ALFPAIController::GetControlledUnit()
{
    return ControlledUnit;
}

void ALFPAIController::SetControlledUnit(ALFPTacticsUnit* NewUnit)
{
    ControlledUnit = NewUnit;
}

void ALFPAIController::SetBehaviorData(ULFPEnemyBehaviorData* NewBehaviorData)
{
    BehaviorData = NewBehaviorData;
}
