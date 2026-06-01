// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Card/LFPCardDataAsset.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

namespace
{
FString GetSkillLogName(const ULFPSkillBase* Skill)
{
    if (!Skill)
    {
        return TEXT("None");
    }

    return Skill->SkillName.IsEmpty() ? Skill->GetName() : Skill->SkillName.ToString();
}

FString GetUnitLogName(const ALFPTacticsUnit* Unit)
{
    return Unit ? Unit->GetName() : TEXT("None");
}

FString GetTileLogName(ALFPHexTile* Tile)
{
    if (!Tile)
    {
        return TEXT("None");
    }

    const FLFPHexCoordinates Coordinates = Tile->GetCoordinates();
    return FString::Printf(TEXT("(%d,%d,%d)"), Coordinates.Q, Coordinates.R, Coordinates.S);
}
}

// Sets default values for this component's properties
ULFPSkillComponent::ULFPSkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    Skills.Empty();
    DefaultAttackSkill = nullptr;
	// ...
}


// Called when the game starts
void ULFPSkillComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void ULFPSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void ULFPSkillComponent::InitializeSkills()
{
    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());
    ULFPGameInstance* GameInstance = OwnerUnit ? Cast<ULFPGameInstance>(OwnerUnit->GetGameInstance()) : nullptr;
    if (!OwnerUnit || !GameInstance || !GameInstance->UnitRegistry)
    {
        InitializeSkillsFromCards(TArray<TSoftObjectPtr<ULFPCardDataAsset>>());
        return;
    }

    FLFPUnitRegistryEntry Entry;
    if (!GameInstance->UnitRegistry->FindEntry(OwnerUnit->UnitTypeID, Entry))
    {
        InitializeSkillsFromCards(TArray<TSoftObjectPtr<ULFPCardDataAsset>>());
        return;
    }

    InitializeSkillsFromCards(Entry.DefaultCarriedCards);
}

void ULFPSkillComponent::InitializeSkillsFromCards(const TArray<TSoftObjectPtr<ULFPCardDataAsset>>& CardDataAssets)
{
    // 清空所有技能
    Skills.Empty();
    DefaultAttackSkill = nullptr;

    ALFPTacticsUnit* OwnerUnit = Cast<ALFPTacticsUnit>(GetOwner());
    if (!OwnerUnit) return;
    const ULFPGameInstance* GameInstance = Cast<ULFPGameInstance>(OwnerUnit->GetGameInstance());

    if (ULFPBuffComponent* BuffComponent = OwnerUnit->GetBuffComponent())
    {
        BuffComponent->ClearPassiveBuffs();
    }

    AddSkillFromCard(GetGlobalAttackCardForOwner(GameInstance, OwnerUnit), OwnerUnit);

    for (const TSoftObjectPtr<ULFPCardDataAsset>& CardData : CardDataAssets)
    {
        AddSkillFromCard(CardData, OwnerUnit);
    }

    if (!DefaultAttackSkill)
    {
        for (ULFPSkillBase* Skill : Skills)
        {
            if (Skill && !Skill->IsPassiveSkill())
            {
                DefaultAttackSkill = Skill;
                break;
            }
        }
    }

    OwnerUnit->RebuildCurrentStatsFromRuntimeSources();

    //// 如果没有默认攻击技能，创建一个
    //if (!DefaultAttackSkill)
    //{
    //    DefaultAttackSkill = NewObject<UAttackSkill>(this);
    //    DefaultAttackSkill->bIsDefaultAttack = true;
    //    DefaultAttackSkill->SkillName = FText::FromString("普通攻击");
    //    Skills.Add(DefaultAttackSkill);
    //}    
}

TSoftObjectPtr<ULFPCardDataAsset> ULFPSkillComponent::GetGlobalAttackCardForOwner(
    const ULFPGameInstance* GameInstance,
    const ALFPTacticsUnit* OwnerUnit) const
{
    if (!GameInstance || !OwnerUnit)
    {
        return TSoftObjectPtr<ULFPCardDataAsset>();
    }

    const FGameplayTag AttackTag = FindFirstOwnerTagWithPrefix(OwnerUnit, TEXT("Unit.Attack."));
    const FString TagName = AttackTag.GetTagName().ToString();
    if (TagName.EndsWith(TEXT(".Ranged")))
    {
        return GameInstance->FallbackRangedAttackCard;
    }
    if (TagName.EndsWith(TEXT(".Magic")))
    {
        return GameInstance->FallbackMagicAttackCard;
    }

    return GameInstance->FallbackMeleeAttackCard;
}

FGameplayTag ULFPSkillComponent::FindFirstOwnerTagWithPrefix(const ALFPTacticsUnit* OwnerUnit, const FString& Prefix) const
{
    if (!OwnerUnit)
    {
        return FGameplayTag();
    }

    const FGameplayTagContainer& Tags = OwnerUnit->GetSpecialTags();
    for (const FGameplayTag& Tag : Tags)
    {
        if (Tag.GetTagName().ToString().StartsWith(Prefix))
        {
            return Tag;
        }
    }

    return FGameplayTag();
}

void ULFPSkillComponent::AddSkillFromCard(const TSoftObjectPtr<ULFPCardDataAsset>& CardData, ALFPTacticsUnit* OwnerUnit)
{
    if (CardData.IsNull() || !OwnerUnit)
    {
        return;
    }

    ULFPCardDataAsset* LoadedCardData = CardData.LoadSynchronous();
    if (!LoadedCardData || !LoadedCardData->IsValidCardData())
    {
        return;
    }

    ULFPSkillBase* NewSkill = NewObject<ULFPSkillBase>(this, LoadedCardData->SkillClass);
    if (!NewSkill)
    {
        return;
    }

    Skills.Add(NewSkill);
    NewSkill->InitSkill(OwnerUnit);
    NewSkill->SkillName = LoadedCardData->DisplayName;
    NewSkill->SkillDescription = LoadedCardData->Description;
    NewSkill->SkillIcon = LoadedCardData->Icon;
    if (LoadedCardData->ActionPointCost != INDEX_NONE)
    {
        NewSkill->ActionPointCost = LoadedCardData->ActionPointCost;
    }

    NewSkill->RegisterPassiveBuffs(OwnerUnit);

    if (!DefaultAttackSkill && NewSkill->bIsDefaultAttack)
    {
        DefaultAttackSkill = NewSkill;
    }
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

    // 技能组件是统一释放入口，这里记日志可以同时覆盖玩家和敌方的实际施法。
    const FString SkillLogName = GetSkillLogName(Skill);
    ALFPTacticsUnit* TargetUnit = TargetTile ? TargetTile->GetUnitOnTile() : nullptr;

    if (!OwnerUnit->IsEnemy())
    {
		// 检查技能是否可用
		if (!Skill->CanExecute(TargetTile))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("技能释放失败: 单位[%s] 技能[%s] 目标格[%s] 目标单位[%s]"),
                *GetUnitLogName(OwnerUnit),
                *SkillLogName,
                *GetTileLogName(TargetTile),
                *GetUnitLogName(TargetUnit));
            return false;
        }
    }

    // 执行技能
    UE_LOG(
        LogTemp,
        Log,
        TEXT("技能释放: 单位[%s] 技能[%s] 目标格[%s] 目标单位[%s] AP[%d]"),
        *GetUnitLogName(OwnerUnit),
        *SkillLogName,
        *GetTileLogName(TargetTile),
        *GetUnitLogName(TargetUnit),
        Skill->ActionPointCost);

    Skill->Execute(TargetTile);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("技能释放完成: 单位[%s] 技能[%s]"),
        *GetUnitLogName(OwnerUnit),
        *SkillLogName);

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
