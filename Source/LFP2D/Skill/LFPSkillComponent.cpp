// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Skill/LFPSkillDataAsset.h"
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
    // 清空所有技能
    Skills.Empty();

    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());
    if (!OwnerUnit || !SkillData) return;

    // 从数据资产创建技能实例
    for (TSubclassOf<ULFPSkillBase> SkillClass : SkillData->AvailableSkills)
    {
        if (SkillClass)
        {
            ULFPSkillBase* NewSkill = NewObject<ULFPSkillBase>(this, SkillClass);
            if (NewSkill)
            {
                Skills.Add(NewSkill);
                NewSkill->InitSkill(OwnerUnit);
            }
        }
    }

    //// 如果没有默认攻击技能，创建一个
    //if (!DefaultAttackSkill)
    //{
    //    DefaultAttackSkill = NewObject<UAttackSkill>(this);
    //    DefaultAttackSkill->bIsDefaultAttack = true;
    //    DefaultAttackSkill->SkillName = FText::FromString("普通攻击");
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

    if (!OwnerUnit->IsEnemy())
    {
		// 检查技能是否可用
		if (!Skill->CanExecute(TargetTile)) return false;
    }

    // 执行技能
    Skill->Execute(TargetTile);

    // 更新技能优先级（降低已使用技能的优先级）
    Skill->OnSkillUsed();

    // 广播事件
    OnSkillExecuted.Broadcast(OwnerUnit, TargetTile);

    return true;
}

ULFPSkillBase* ULFPSkillComponent::GetDefaultAttackSkill() const
{
    return DefaultAttackSkill;
}

void ULFPSkillComponent::OnTurnStarted()
{
    // 更新所有技能的冷却
    for (ULFPSkillBase* Skill : Skills)
    {
        if (Skill)
        {
            Skill->OnTurnStart();
        }
    }
}
