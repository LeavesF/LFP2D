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
    // ������м���
    Skills.Empty();

    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());
    if (!OwnerUnit || !SkillData) return;

    // �������ʲ���������ʵ��
    for (TSubclassOf<ULFPSkillBase> SkillClass : SkillData->AvailableSkills)
    {
        if (SkillClass)
        {
            ULFPSkillBase* NewSkill = NewObject<ULFPSkillBase>(this, SkillClass);
            if (NewSkill)
            {
                Skills.Add(NewSkill);

                // ����Ĭ�Ϲ�������
                if (NewSkill->bIsDefaultAttack)
                {
                    DefaultAttackSkill = NewSkill;
                }
            }
        }
    }

    //// ���û��Ĭ�Ϲ������ܣ�����һ��
    //if (!DefaultAttackSkill)
    //{
    //    DefaultAttackSkill = NewObject<UAttackSkill>(this);
    //    DefaultAttackSkill->bIsDefaultAttack = true;
    //    DefaultAttackSkill->SkillName = FText::FromString("��ͨ����");
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
        if (Skill && Skill->CanExecute(OwnerUnit))
        {
            AvailableSkills.Add(Skill);
        }
    }

    return AvailableSkills;
}

bool ULFPSkillComponent::ExecuteSkill(ULFPSkillBase* Skill, ALFPHexTile* TargetTile)
{
    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());
    if (!OwnerUnit || !Skill || !TargetTile) return false;

    // ��鼼���Ƿ����
    if (!Skill->CanExecute(OwnerUnit)) return false;

    // ִ�м���
    Skill->Execute(OwnerUnit, TargetTile);

    // �����¼�
    OnSkillExecuted.Broadcast(OwnerUnit, TargetTile);

    return true;
}

ULFPSkillBase* ULFPSkillComponent::GetDefaultAttackSkill() const
{
    return DefaultAttackSkill;
}

void ULFPSkillComponent::OnTurnStarted()
{
    // �������м��ܵ���ȴ
    for (ULFPSkillBase* Skill : Skills)
    {
        if (Skill)
        {
            Skill->OnTurnStart();
        }
    }
}