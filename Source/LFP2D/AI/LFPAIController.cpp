// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/AI/LFPEnemyBehaviorData.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Kismet/GameplayStatics.h"
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
    if (!ControlledUnit || !GridManager) return nullptr;

    TArray<AActor*> PlayerUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), PlayerUnits);

    ALFPTacticsUnit* BestTarget = nullptr;
    float BestThreatValue = -MAX_FLT;

    for (AActor* Actor : PlayerUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (Unit && Unit->IsAlive() && Unit->IsAlly())
        {
            float ThreatValue = CalculateThreatValue(Unit);
            if (ThreatValue > BestThreatValue)
            {
                BestThreatValue = ThreatValue;
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

    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(ControlledUnit->GetCurrentTile(), ControlledUnit->GetCurrentMovePoints());

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
    FEnemyActionPlan Plan;
    Plan.EnemyUnit = ControlledUnit;

    if (!ControlledUnit || !GridManager)
    {
        return Plan;
    }

    if (PreAllocatedSkill)
    {
        FEnemySkillPlanCandidate Candidate;
        if (BuildBestSkillPlanCandidate(PreAllocatedSkill, 0, Candidate))
        {
            return Candidate.ToActionPlan();
        }
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

    if (BestCandidate)
    {
        return BestCandidate->ToActionPlan();
    }

    return CreateMovementOnlyPlan(nullptr);
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
        TArray<ALFPHexTile*> SelfTargetTiles = GridManager->GetTilesInRange(CurrentTile, ControlledUnit->GetCurrentMovePoints());
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
        FallbackTarget = FindBestTarget();
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
    OutCandidate.HatredValue = TargetUnit ? Skill->CalculateHatredValue(ControlledUnit, TargetUnit) : 0.0f;
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

    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(CurrentTile, ControlledUnit->GetCurrentMovePoints());
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
