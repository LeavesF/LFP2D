#include "LFP2D/Card/LFPBattleCardComponent.h"

#include "LFP2D/Card/LFPCardDataAsset.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/SkillInstance/LFPSkill_BasicMeleeAttack.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPBattleCardComponent::ULFPBattleCardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULFPBattleCardComponent::InitializeBattleDeck(ULFPGameInstance* GameInstance, const TArray<ALFPTacticsUnit*>& DeployedUnits)
{
	DrawPile.Empty();
	Hand.Empty();
	DiscardPile.Empty();
	ExhaustPile.Empty();
	NextCardInstanceID = 1;

	AddPlayerDeckCards(GameInstance);
	AddUnitCards(GameInstance, DeployedUnits);

	ShuffleDrawPile();
	bInitialized = true;

	DrawCards(OpeningDrawCount);
}

int32 ULFPBattleCardComponent::DrawCards(int32 Count)
{
	if (Count <= 0)
	{
		return 0;
	}

	int32 DrawnCount = 0;
	for (int32 Index = 0; Index < Count && Hand.Num() < MaxHandSize; ++Index)
	{
		if (DrawPile.IsEmpty())
		{
			ShuffleDiscardIntoDrawPile();
		}

		if (DrawPile.IsEmpty())
		{
			break;
		}

		FLFPCardInstance Card = DrawPile.Pop(EAllowShrinking::No);
		Card.CurrentPile = ELFPCardPile::Hand;
		Hand.Add(Card);
		DrawnCount++;
	}

	return DrawnCount;
}

int32 ULFPBattleCardComponent::DrawUpToHandLimit()
{
	return DrawCards(FMath::Max(0, MaxHandSize - Hand.Num()));
}

TArray<FLFPCardInstance> ULFPBattleCardComponent::GetPlayableHandCardsForUnit(ALFPTacticsUnit* Unit)
{
	TArray<FLFPCardInstance> Result;
	if (!Unit)
	{
		return Result;
	}

	for (FLFPCardInstance& Card : Hand)
	{
		if (!Card.IsValid())
		{
			continue;
		}

		if (!Unit->CanUseCard(Card))
		{
			continue;
		}

		PrepareCardForUnit(Card, Unit);
		Result.Add(Card);
	}

	return Result;
}

bool ULFPBattleCardComponent::FinishPlayingCard(int32 CardInstanceID)
{
	// 先从手牌查找。
	const int32 HandIndex = Hand.IndexOfByPredicate([CardInstanceID](const FLFPCardInstance& Card)
	{
		return Card.InstanceID == CardInstanceID;
	});

	if (HandIndex != INDEX_NONE)
	{
		const ELFPCardPile Destination = Hand[HandIndex].Definition.DestinationAfterPlay;
		return MoveHandCardToPile(CardInstanceID, Destination);
	}

	// 从待执行区查找。
	return false;
}

bool ULFPBattleCardComponent::MoveHandCardToPile(int32 CardInstanceID, ELFPCardPile TargetPile)
{
	if (TargetPile == ELFPCardPile::Hand)
	{
		return false;
	}

	const int32 HandIndex = Hand.IndexOfByPredicate([CardInstanceID](const FLFPCardInstance& Card)
	{
		return Card.InstanceID == CardInstanceID;
	});

	if (HandIndex == INDEX_NONE)
	{
		return false;
	}

	TArray<FLFPCardInstance>* TargetCards = GetPileMutable(TargetPile);
	if (!TargetCards)
	{
		return false;
	}

	FLFPCardInstance Card = Hand[HandIndex];
	Hand.RemoveAt(HandIndex);
	Card.CurrentPile = TargetPile;
	TargetCards->Add(Card);
	return true;
}

bool ULFPBattleCardComponent::AddCardToDrawPile(const FLFPCardDefinition& Definition, ALFPTacticsUnit* SourceUnit,
	FGameplayTag RequiredTagOverride)
{
	if (!Definition.SkillClass)
	{
		return false;
	}

	ULFPSkillBase* RuntimeSkill = NewObject<ULFPSkillBase>(this, Definition.SkillClass);
	if (!RuntimeSkill)
	{
		return false;
	}

	RuntimeSkill->InitSkill(SourceUnit);
	const FString SkillClassName = Definition.SkillClass->GetName();

	FLFPCardInstance Card;
	Card.InstanceID = NextCardInstanceID++;
	Card.SourceUnit = SourceUnit;
	Card.RuntimeSkill = RuntimeSkill;
	Card.CurrentPile = ELFPCardPile::DrawPile;
	Card.Definition = Definition;
	if (Card.Definition.CardID.IsNone())
	{
		Card.Definition.CardID = FName(*FString::Printf(TEXT("%s_%d"), *SkillClassName, Card.InstanceID));
	}
	if (Card.Definition.DisplayName.IsEmpty())
	{
		Card.Definition.DisplayName = RuntimeSkill->SkillName;
	}
	if (Card.Definition.Description.IsEmpty())
	{
		Card.Definition.Description = RuntimeSkill->SkillDescription;
	}
	if (!Card.Definition.Icon)
	{
		Card.Definition.Icon = RuntimeSkill->SkillIcon;
	}
	if (Card.Definition.ActionPointCost < 0)
	{
		Card.Definition.ActionPointCost = RuntimeSkill->ActionPointCost;
	}
	else
	{
		Card.Definition.ActionPointCost = FMath::Max(0, Card.Definition.ActionPointCost);
		RuntimeSkill->ActionPointCost = Card.Definition.ActionPointCost;
	}
	RuntimeSkill->SkillName = Card.Definition.DisplayName;
	RuntimeSkill->SkillDescription = Card.Definition.Description;
	RuntimeSkill->SkillIcon = Card.Definition.Icon;
	Card.Definition.RequiredTag = ResolveRequiredTag(Card.Definition, SourceUnit, RequiredTagOverride);

	DrawPile.Add(Card);
	return true;
}

bool ULFPBattleCardComponent::AddCardDataToDrawPile(const TSoftObjectPtr<ULFPCardDataAsset>& CardData,
	ALFPTacticsUnit* SourceUnit, FGameplayTag RequiredTagOverride)
{
	ULFPCardDataAsset* LoadedCardData = CardData.LoadSynchronous();
	if (!LoadedCardData || !LoadedCardData->IsValidCardData())
	{
		return false;
	}

	return AddCardToDrawPile(LoadedCardData->BuildCardDefinition(), SourceUnit, RequiredTagOverride);
}

bool ULFPBattleCardComponent::AddCardToDrawPile(TSubclassOf<ULFPSkillBase> SkillClass, ALFPTacticsUnit* SourceUnit,
	ELFPCardCategory Category, FGameplayTag RequiredTag)
{
	if (!SkillClass)
	{
		return false;
	}

	FLFPCardDefinition Definition;
	Definition.SkillClass = SkillClass;
	Definition.DestinationAfterPlay = ELFPCardPile::DiscardPile;
	Definition.CardCategory = Category;
	Definition.RequiredTag = RequiredTag;
	return AddCardToDrawPile(Definition, SourceUnit);
}

void ULFPBattleCardComponent::AddPlayerDeckCards(ULFPGameInstance* GameInstance)
{
	if (!GameInstance)
	{
		return;
	}

	if (!GameInstance->PlayerDeckCards.IsEmpty())
	{
		for (const TSoftObjectPtr<ULFPCardDataAsset>& CardData : GameInstance->PlayerDeckCards)
		{
			AddCardDataToDrawPile(CardData, nullptr);
		}
		return;
	}

	for (TSubclassOf<ULFPSkillBase> SkillClass : GameInstance->PlayerDeckCardSkillClasses)
	{
		// 玩家牌库卡默认为完全通用型；无目标型由蓝图测 Skill.TargetType 决定。
		AddCardToDrawPile(SkillClass, nullptr, ELFPCardCategory::FullyGeneric);
	}
}

void ULFPBattleCardComponent::AddUnitCards(ULFPGameInstance* GameInstance, const TArray<ALFPTacticsUnit*>& DeployedUnits)
{
	for (int32 Index = 0; Index < DeployedUnits.Num(); ++Index)
	{
		ALFPTacticsUnit* Unit = DeployedUnits[Index];
		if (!Unit)
		{
			continue;
		}

		// 单位注册表中的 DefaultCarriedCardSkillClasses 视为种族专属型。
		if (GameInstance && GameInstance->UnitRegistry)
		{
			FLFPUnitRegistryEntry Entry;
			if (GameInstance->UnitRegistry->FindEntry(Unit->UnitTypeID, Entry))
			{
				if (!Entry.DefaultCarriedCards.IsEmpty())
				{
					AddConfiguredUnitCards(Unit, Entry.DefaultCarriedCards);
				}
				else
				{
					AddConfiguredUnitCards(Unit, Entry.DefaultCarriedCardSkillClasses,
						ELFPCardCategory::RaceSpecific);
				}
			}
		}
	}

	// 普攻卡按攻击Tag分组共享生成，不再按单位独立创建。
	AddSharedAttackCards(DeployedUnits);
}

void ULFPBattleCardComponent::AddConfiguredUnitCards(ALFPTacticsUnit* Unit,
	const TArray<TSoftObjectPtr<ULFPCardDataAsset>>& Cards)
{
	for (const TSoftObjectPtr<ULFPCardDataAsset>& CardData : Cards)
	{
		AddCardDataToDrawPile(CardData, Unit);
	}
}

void ULFPBattleCardComponent::AddConfiguredUnitCards(ALFPTacticsUnit* Unit,
	const TArray<TSubclassOf<ULFPSkillBase>>& CardSkillClasses, ELFPCardCategory Category)
{
	for (TSubclassOf<ULFPSkillBase> SkillClass : CardSkillClasses)
	{
		// 种族专属型使用单位的种族Tag；通用型后续可额外指定Tag。
		FGameplayTag Tag;
		if (Category == ELFPCardCategory::RaceSpecific)
		{
			// 从单位SpecialTags中取第一个种族Tag（如 Unit.Race.Dragon）
			const FGameplayTagContainer& Tags = Unit->GetSpecialTags();
			for (const FGameplayTag& T : Tags)
			{
				if (T.GetTagName().ToString().StartsWith(TEXT("Unit.Race.")))
				{
					Tag = T;
					break;
				}
			}
		}

		AddCardToDrawPile(SkillClass, Unit, Category, Tag);
	}
}

void ULFPBattleCardComponent::AddSharedAttackCards(const TArray<ALFPTacticsUnit*>& DeployedUnits)
{
	// 按攻击Tag分组，每种Tag生成等于该Tag单位数量的普攻卡。
	TMap<FGameplayTag, int32> AttackTagCounts;

	for (ALFPTacticsUnit* Unit : DeployedUnits)
	{
		if (!Unit)
		{
			continue;
		}

		const FGameplayTagContainer& Tags = Unit->GetSpecialTags();
		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.GetTagName().ToString().StartsWith(TEXT("Unit.Attack.")))
			{
				AttackTagCounts.FindOrAdd(Tag)++;
				break;
			}
		}
	}

	// 如果单位都没有攻击Tag，为每个单位创建无Tag限制的普攻卡（用近战兜底）。
	if (AttackTagCounts.IsEmpty())
	{
		for (ALFPTacticsUnit* Unit : DeployedUnits)
		{
			if (!Unit)
			{
				continue;
			}

			if (!FallbackMeleeAttackCard.IsNull() && AddCardDataToDrawPile(FallbackMeleeAttackCard, nullptr))
			{
				continue;
			}

			TSubclassOf<ULFPSkillBase> AttackClass = FallbackMeleeAttackClass;
			if (ULFPSkillBase* DefaultAttackSkill = Unit->GetDefaultAttackSkill())
			{
				AttackClass = DefaultAttackSkill->GetClass();
			}

			AddCardToDrawPile(AttackClass, nullptr, ELFPCardCategory::GeneralAttack);
		}
		return;
	}

	// 按攻击Tag生成共享普攻卡。
	for (const auto& Pair : AttackTagCounts)
	{
		const FGameplayTag& AttackTag = Pair.Key;
		const int32 Count = Pair.Value;
		const FString TagName = AttackTag.GetTagName().ToString();
		TSoftObjectPtr<ULFPCardDataAsset> AttackCard;

		// 根据攻击Tag选择对应的兜底技能类。
		TSubclassOf<ULFPSkillBase> AttackClass;
		if (TagName.EndsWith(TEXT(".Melee")))
		{
			AttackCard = FallbackMeleeAttackCard;
			AttackClass = FallbackMeleeAttackClass;
		}
		else if (TagName.EndsWith(TEXT(".Ranged")))
		{
			AttackCard = FallbackRangedAttackCard;
			AttackClass = FallbackRangedAttackClass;
		}
		else if (TagName.EndsWith(TEXT(".Magic")))
		{
			AttackCard = FallbackMagicAttackCard;
			AttackClass = FallbackMagicAttackClass;
		}
		else
		{
			AttackCard = FallbackMeleeAttackCard;
			AttackClass = FallbackMeleeAttackClass;
		}

		for (int32 i = 0; i < Count; ++i)
		{
			if (!AttackCard.IsNull() && AddCardDataToDrawPile(AttackCard, nullptr, AttackTag))
			{
				continue;
			}

			AddCardToDrawPile(AttackClass, nullptr,
				ELFPCardCategory::GeneralAttack, AttackTag);
		}
	}
}

FGameplayTag ULFPBattleCardComponent::ResolveRequiredTag(const FLFPCardDefinition& Definition,
	ALFPTacticsUnit* SourceUnit, FGameplayTag RequiredTagOverride) const
{
	if (RequiredTagOverride.IsValid())
	{
		return RequiredTagOverride;
	}

	if (Definition.RequiredTag.IsValid())
	{
		return Definition.RequiredTag;
	}

	if (Definition.CardCategory == ELFPCardCategory::RaceSpecific)
	{
		return FindFirstUnitTagWithPrefix(SourceUnit, TEXT("Unit.Race."));
	}

	return FGameplayTag();
}

FGameplayTag ULFPBattleCardComponent::FindFirstUnitTagWithPrefix(ALFPTacticsUnit* Unit, const FString& Prefix) const
{
	if (!Unit)
	{
		return FGameplayTag();
	}

	const FGameplayTagContainer& Tags = Unit->GetSpecialTags();
	for (const FGameplayTag& Tag : Tags)
	{
		if (Tag.GetTagName().ToString().StartsWith(Prefix))
		{
			return Tag;
		}
	}

	return FGameplayTag();
}

void ULFPBattleCardComponent::PrepareCardForUnit(FLFPCardInstance& Card, ALFPTacticsUnit* Unit)
{
	if (!Card.RuntimeSkill || !Unit)
	{
		return;
	}

	if (Card.RuntimeSkill->Owner == Unit)
	{
		return;
	}

	// 非单位专属卡（SourceUnit==nullptr）绑定到当前选中单位。
	// 单位专属卡保留源单位作为Owner。
	Card.RuntimeSkill->Owner = Unit;
	Card.RuntimeSkill->UpdateSkillRange();
}

void ULFPBattleCardComponent::ShuffleDrawPile()
{
	for (int32 Index = DrawPile.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		DrawPile.Swap(Index, SwapIndex);
	}
}

void ULFPBattleCardComponent::ShuffleDiscardIntoDrawPile()
{
	if (DiscardPile.IsEmpty())
	{
		return;
	}

	for (FLFPCardInstance& Card : DiscardPile)
	{
		Card.CurrentPile = ELFPCardPile::DrawPile;
		DrawPile.Add(Card);
	}

	DiscardPile.Empty();
	ShuffleDrawPile();
}

TArray<FLFPCardInstance>* ULFPBattleCardComponent::GetPileMutable(ELFPCardPile Pile)
{
	switch (Pile)
	{
	case ELFPCardPile::DrawPile:
		return &DrawPile;
	case ELFPCardPile::Hand:
		return &Hand;
	case ELFPCardPile::DiscardPile:
		return &DiscardPile;
	case ELFPCardPile::ExhaustPile:
		return &ExhaustPile;
	default:
		return nullptr;
	}
}

bool ULFPBattleCardComponent::MoveCardToPile(int32 CardInstanceID, ELFPCardPile TargetPile)
{
	if (TargetPile == ELFPCardPile::Hand)
	{
		return false;
	}

	// 在所有牌堆中查找该卡牌。
	static const TArray<ELFPCardPile> AllPiles = {
		ELFPCardPile::DrawPile, ELFPCardPile::Hand,
		ELFPCardPile::DiscardPile, ELFPCardPile::ExhaustPile
	};

	ELFPCardPile SourcePile = ELFPCardPile::DrawPile;
	int32 SourceIndex = INDEX_NONE;
	FLFPCardInstance FoundCard;

	for (const ELFPCardPile Pile : AllPiles)
	{
		TArray<FLFPCardInstance>* Cards = GetPileMutable(Pile);
		if (!Cards) continue;

		const int32 Idx = Cards->IndexOfByPredicate([CardInstanceID](const FLFPCardInstance& C)
		{
			return C.InstanceID == CardInstanceID;
		});

		if (Idx != INDEX_NONE)
		{
			SourcePile = Pile;
			SourceIndex = Idx;
			FoundCard = (*Cards)[Idx];
			Cards->RemoveAt(Idx);
			break;
		}
	}

	if (SourceIndex == INDEX_NONE)
	{
		return false;
	}

	TArray<FLFPCardInstance>* TargetCards = GetPileMutable(TargetPile);
	if (!TargetCards)
	{
		// 失败的移动放回原牌堆。
		TArray<FLFPCardInstance>* SourceCards = GetPileMutable(SourcePile);
		if (SourceCards) SourceCards->Add(FoundCard);
		return false;
	}

	FoundCard.CurrentPile = TargetPile;
	TargetCards->Add(FoundCard);
	return true;
}
