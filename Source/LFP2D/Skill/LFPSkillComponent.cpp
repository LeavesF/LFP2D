// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Skill/LFPSkillDataAsset.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

// Sets default values for this component's properties
ULFPSkillComponent::ULFPSkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    DefaultAttackSkill = nullptr;
	// ...
}


// Called when the game starts
void ULFPSkillComponent::BeginPlay()
{
	Super::BeginPlay();

    InitializeSkills();
}


// Called every frame
void ULFPSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void ULFPSkillComponent::InitializeSkills()
{
    // 娓呯┖鎵€鏈夋妧鑳?
    Skills.Empty();

    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());
    if (!OwnerUnit || !SkillData) return;

    if (ULFPBuffComponent* BuffComponent = OwnerUnit->GetBuffComponent())
    {
        BuffComponent->ClearPersistentBuffs();
    }

    // 浠庢暟鎹祫浜у垱寤烘妧鑳藉疄渚?
    for (TSubclassOf<ULFPSkillBase> SkillClass : SkillData->AvailableSkills)
    {
        if (SkillClass)
        {
            ULFPSkillBase* NewSkill = NewObject<ULFPSkillBase>(this, SkillClass);
            if (NewSkill)
            {
                Skills.Add(NewSkill);
                NewSkill->InitSkill(OwnerUnit);
                NewSkill->RegisterPassiveBuffs(OwnerUnit);
            }
        }
    }

    OwnerUnit->RebuildCurrentStatsFromRuntimeSources();

    //// 濡傛灉娌℃湁榛樿鏀诲嚮鎶€鑳斤紝鍒涘缓涓€涓?
    //if (!DefaultAttackSkill)
    //{
    //    DefaultAttackSkill = NewObject<UAttackSkill>(this);
    //    DefaultAttackSkill->bIsDefaultAttack = true;
    //    DefaultAttackSkill->SkillName = FText::FromString("鏅€氭敾鍑?);
    //    Skills.Add(DefaultAttackSkill);
    //}
}

TArray<ULFPSkillBase*> ULFPSkillComponent::GetAvailableSkills() const
{
    TArray<ULFPSkillBase*> AvailableSkills;
    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());

    if (!OwnerUnit) return AvailableSkills;

    for (ULFPSkillBase* Skill : Skills)
    {
        if (Skill)
        {
            AvailableSkills.Add(Skill);
        }
    }

    return AvailableSkills;
}

bool ULFPSkillComponent::ExecuteSkill(ULFPSkillBase* Skill, ALFPHexTile* TargetTile)
{
    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());
    if (!OwnerUnit || !Skill) return false;
    if (Skill->IsPassiveSkill()) return false;

    // 统一动作入口：这里只负责校验、扣费和挂起动作，真正效果延后到动画 commit 帧。
    const bool bActionPointsPreConsumed = ShouldTreatActionPointsAsPreConsumed(OwnerUnit, Skill);
    const bool bCanExecute = bActionPointsPreConsumed
        ? Skill->CanExecuteIgnoringActionPoints(TargetTile)
        : Skill->CanExecute(TargetTile);
    if (!bCanExecute) return false;
    if (!OwnerUnit->CanBeginSkillAction()) return false;

    if (!bActionPointsPreConsumed)
    {
        OwnerUnit->ConsumeActionPoints(Skill->ActionPointCost);
    }

    if (!OwnerUnit->BeginSkillAction(Skill, TargetTile, bActionPointsPreConsumed || Skill->ActionPointCost > 0))
    {
        return false;
    }

    // 动作已经成功挂起，此时只更新技能优先级，不做即时结算。
    Skill->OnSkillUsed();
    return true;
}

ULFPSkillBase* ULFPSkillComponent::GetDefaultAttackSkill() const
{
    return DefaultAttackSkill;
}

void ULFPSkillComponent::OnTurnStarted()
{
    // 鏇存柊鎵€鏈夋妧鑳界殑鍐峰嵈
    for (ULFPSkillBase* Skill : Skills)
    {
        if (Skill)
        {
            Skill->OnTurnStart();
        }
    }
}

void ULFPSkillComponent::NotifySkillCommitted(ALFPHexTile* TargetTile)
{
    if (ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner()))
    {
        // 沿用原有广播接口，但语义改为“动画已提交，技能已真正生效”。
        OnSkillExecuted.Broadcast(OwnerUnit, TargetTile);
    }
}

bool ULFPSkillComponent::ShouldTreatActionPointsAsPreConsumed(ALFPTacticsUnit* OwnerUnit, ULFPSkillBase* Skill) const
{
    if (!OwnerUnit || !Skill || !OwnerUnit->IsEnemy())
    {
        return false;
    }

    const ALFPTurnManager* TurnManager = OwnerUnit->GetTurnManager();
    if (!TurnManager || TurnManager->GetCurrentPhase() != EBattlePhase::BP_ActionPhase)
    {
        return false;
    }

    // 敌方若已在规划阶段预扣 AP，这里就不要重复扣费。
    const FEnemyActionPlan& Plan = TurnManager->GetPlanForEnemy(OwnerUnit);
    return Plan.bIsValid && Plan.PlannedSkill == Skill && Skill->ActionPointCost > 0;
}
