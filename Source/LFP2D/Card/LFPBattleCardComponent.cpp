#include "LFP2D/Card/LFPBattleCardComponent.h"

#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/SkillInstance/LFPSkill_BasicMeleeAttack.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPBattleCardComponent::ULFPBattleCardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	FallbackDefaultAttackSkillClass = ULFPSkill_BasicMeleeAttack::StaticClass();
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

	/* 战斗刚开始先抽起始手牌，后续玩家行动阶段只负责补到手牌上限。 */
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

		/* 单位携带卡只允许来源单位打出；公共牌库卡会在这里绑定到当前选中单位。 */
		if (Card.SourceUnit && Card.SourceUnit != Unit)
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
	const int32 HandIndex = Hand.IndexOfByPredicate([CardInstanceID](const FLFPCardInstance& Card)
	{
		return Card.InstanceID == CardInstanceID;
	});

	if (HandIndex == INDEX_NONE)
	{
		return false;
	}

	const ELFPCardPile Destination = Hand[HandIndex].Definition.DestinationAfterPlay;
	return MoveHandCardToPile(CardInstanceID, Destination);
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

void ULFPBattleCardComponent::AddCardToDrawPile(TSubclassOf<ULFPSkillBase> SkillClass, ALFPTacticsUnit* SourceUnit)
{
	if (!SkillClass)
	{
		return;
	}

	ULFPSkillBase* RuntimeSkill = NewObject<ULFPSkillBase>(this, SkillClass);
	if (!RuntimeSkill)
	{
		return;
	}

	const FString SkillClassName = SkillClass->GetName();
	RuntimeSkill->InitSkill(SourceUnit);

	FLFPCardInstance Card;
	Card.InstanceID = NextCardInstanceID++;
	Card.SourceUnit = SourceUnit;
	Card.RuntimeSkill = RuntimeSkill;
	Card.CurrentPile = ELFPCardPile::DrawPile;
	Card.Definition.CardID = FName(*FString::Printf(TEXT("%s_%d"), *SkillClassName, Card.InstanceID));
	Card.Definition.DisplayName = RuntimeSkill->SkillName;
	Card.Definition.Description = RuntimeSkill->SkillDescription;
	Card.Definition.Icon = RuntimeSkill->SkillIcon;
	Card.Definition.SkillClass = SkillClass;
	Card.Definition.DestinationAfterPlay = ELFPCardPile::DiscardPile;

	DrawPile.Add(Card);
}

void ULFPBattleCardComponent::AddPlayerDeckCards(ULFPGameInstance* GameInstance)
{
	if (!GameInstance)
	{
		return;
	}

	for (TSubclassOf<ULFPSkillBase> SkillClass : GameInstance->PlayerDeckCardSkillClasses)
	{
		AddCardToDrawPile(SkillClass, nullptr);
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

		if (GameInstance && GameInstance->UnitRegistry)
		{
			FLFPUnitRegistryEntry Entry;
			if (GameInstance->UnitRegistry->FindEntry(Unit->UnitTypeID, Entry))
			{
				AddConfiguredUnitCards(Unit, Entry.DefaultCarriedCardSkillClasses);
			}
		}

		AddDefaultAttackCard(Unit);
	}
}

void ULFPBattleCardComponent::AddConfiguredUnitCards(ALFPTacticsUnit* Unit, const TArray<TSubclassOf<ULFPSkillBase>>& CardSkillClasses)
{
	for (TSubclassOf<ULFPSkillBase> SkillClass : CardSkillClasses)
	{
		AddCardToDrawPile(SkillClass, Unit);
	}
}

void ULFPBattleCardComponent::AddDefaultAttackCard(ALFPTacticsUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	TSubclassOf<ULFPSkillBase> DefaultAttackClass = FallbackDefaultAttackSkillClass;
	if (ULFPSkillBase* DefaultAttackSkill = Unit->GetDefaultAttackSkill())
	{
		DefaultAttackClass = DefaultAttackSkill->GetClass();
	}

	AddCardToDrawPile(DefaultAttackClass, Unit);
}

void ULFPBattleCardComponent::PrepareCardForUnit(FLFPCardInstance& Card, ALFPTacticsUnit* Unit)
{
	if (!Card.RuntimeSkill)
	{
		return;
	}

	ALFPTacticsUnit* DesiredOwner = Card.SourceUnit ? Card.SourceUnit.Get() : Unit;
	if (!DesiredOwner || Card.RuntimeSkill->Owner == DesiredOwner)
	{
		return;
	}

	/* 公共牌库卡没有固定来源，展示和使用前绑定到当前选中单位。 */
	Card.RuntimeSkill->Owner = DesiredOwner;
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

	/* 抽牌堆耗尽时，弃牌堆洗回抽牌堆，销毁牌堆不参与循环。 */
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
