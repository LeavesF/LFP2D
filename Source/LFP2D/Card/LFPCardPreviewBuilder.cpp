#include "LFP2D/Card/LFPCardPreviewBuilder.h"

#include "LFP2D/Card/LFPCardDataAsset.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"

namespace
{
FGameplayTag FindFirstTagWithPrefix(const FGameplayTagContainer& Tags, const FString& Prefix)
{
	for (const FGameplayTag& Tag : Tags)
	{
		if (Tag.GetTagName().ToString().StartsWith(Prefix))
		{
			return Tag;
		}
	}

	return FGameplayTag();
}

bool AddPreviewCardDefinition(
	const FLFPCardDefinition& InDefinition,
	UObject* Outer,
	int32 MaxCards,
	TArray<FLFPCardInstance>& OutCards)
{
	if (!Outer || !InDefinition.SkillClass || MaxCards <= 0 || OutCards.Num() >= MaxCards)
	{
		return false;
	}

	ULFPSkillBase* RuntimeSkill = NewObject<ULFPSkillBase>(Outer, InDefinition.SkillClass);
	if (!RuntimeSkill)
	{
		return false;
	}

	RuntimeSkill->InitSkill(nullptr);

	FLFPCardInstance Card;
	Card.InstanceID = OutCards.Num() + 1;
	Card.SourceUnit = nullptr;
	Card.RuntimeSkill = RuntimeSkill;
	Card.CurrentPile = ELFPCardPile::DrawPile;
	Card.Definition = InDefinition;

	if (Card.Definition.CardID.IsNone())
	{
		Card.Definition.CardID = FName(*FString::Printf(TEXT("%s_%d"),
			*InDefinition.SkillClass->GetName(),
			Card.InstanceID));
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

	OutCards.Add(Card);
	return true;
}

bool AddPreviewCardData(
	const TSoftObjectPtr<ULFPCardDataAsset>& CardData,
	UObject* Outer,
	int32 MaxCards,
	TArray<FLFPCardInstance>& OutCards,
	const FGameplayTag RequiredTagOverride = FGameplayTag())
{
	ULFPCardDataAsset* LoadedCardData = CardData.LoadSynchronous();
	if (!LoadedCardData || !LoadedCardData->IsValidCardData())
	{
		return false;
	}

	FLFPCardDefinition Definition = LoadedCardData->BuildCardDefinition();
	if (RequiredTagOverride.IsValid())
	{
		Definition.RequiredTag = RequiredTagOverride;
	}

	return AddPreviewCardDefinition(Definition, Outer, MaxCards, OutCards);
}

bool AddPreviewCardClass(
	TSubclassOf<ULFPSkillBase> SkillClass,
	UObject* Outer,
	int32 MaxCards,
	TArray<FLFPCardInstance>& OutCards,
	ELFPCardCategory Category,
	const FGameplayTag RequiredTag = FGameplayTag())
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
	return AddPreviewCardDefinition(Definition, Outer, MaxCards, OutCards);
}

void AddSharedAttackCardPreview(
	const FLFPUnitRegistryEntry& UnitDefinition,
	const FLFPCardPreviewAttackDefaults& AttackDefaults,
	UObject* Outer,
	int32 MaxCards,
	TArray<FLFPCardInstance>& OutCards)
{
	const FGameplayTag AttackTag = FindFirstTagWithPrefix(UnitDefinition.SpecialTags, TEXT("Unit.Attack."));
	const FString AttackTagName = AttackTag.GetTagName().ToString();

	TSoftObjectPtr<ULFPCardDataAsset> AttackCard = AttackDefaults.MeleeAttackCard;
	TSubclassOf<ULFPSkillBase> AttackClass = AttackDefaults.MeleeAttackClass;
	if (AttackTagName.EndsWith(TEXT(".Ranged")))
	{
		AttackCard = AttackDefaults.RangedAttackCard;
		AttackClass = AttackDefaults.RangedAttackClass;
	}
	else if (AttackTagName.EndsWith(TEXT(".Magic")))
	{
		AttackCard = AttackDefaults.MagicAttackCard;
		AttackClass = AttackDefaults.MagicAttackClass;
	}

	if (!AttackCard.IsNull() && AddPreviewCardData(AttackCard, Outer, MaxCards, OutCards, AttackTag))
	{
		return;
	}

	AddPreviewCardClass(AttackClass, Outer, MaxCards, OutCards, ELFPCardCategory::GeneralAttack, AttackTag);
}

void AddCarriedCardPreviews(
	const FLFPUnitRegistryEntry& UnitDefinition,
	UObject* Outer,
	int32 MaxCards,
	TArray<FLFPCardInstance>& OutCards)
{
	const FGameplayTag RaceTag = FindFirstTagWithPrefix(UnitDefinition.SpecialTags, TEXT("Unit.Race."));

	for (const TSoftObjectPtr<ULFPCardDataAsset>& CardData : UnitDefinition.DefaultCarriedCards)
	{
		ULFPCardDataAsset* LoadedCardData = CardData.LoadSynchronous();
		if (!LoadedCardData || !LoadedCardData->IsValidCardData())
		{
			continue;
		}

		FLFPCardDefinition Definition = LoadedCardData->BuildCardDefinition();
		if (Definition.CardCategory == ELFPCardCategory::RaceSpecific && !Definition.RequiredTag.IsValid())
		{
			Definition.RequiredTag = RaceTag;
		}

		AddPreviewCardDefinition(Definition, Outer, MaxCards, OutCards);
		if (OutCards.Num() >= MaxCards)
		{
			break;
		}
	}
}
}

void FLFPCardPreviewBuilder::BuildUnitCardsPreview(
	const FLFPUnitRegistryEntry& UnitDefinition,
	const FLFPCardPreviewAttackDefaults& AttackDefaults,
	UObject* Outer,
	int32 MaxCards,
	TArray<FLFPCardInstance>& OutCards)
{
	OutCards.Reset();

	if (!Outer || MaxCards <= 0)
	{
		return;
	}

	AddSharedAttackCardPreview(UnitDefinition, AttackDefaults, Outer, MaxCards, OutCards);
	if (OutCards.Num() >= MaxCards)
	{
		return;
	}

	AddCarriedCardPreviews(UnitDefinition, Outer, MaxCards, OutCards);
}
