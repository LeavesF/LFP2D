#include "LFP2D/UI/Fighting/LFPCurrentUnitInfoWidget.h"

#include "LFP2D/Card/LFPBattleCardComponent.h"
#include "LFP2D/Card/LFPCardDataAsset.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/UI/Fighting/LFPBuffIconWidget.h"
#include "LFP2D/UI/Fighting/LFPCardItemWidget.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "TimerManager.h"

void ULFPCurrentUnitInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheDefaultStatTextColors();
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULFPCurrentUnitInfoWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	UnbindFromTurnManager();
	UnbindFromUnit();

	Super::NativeDestruct();
}

void ULFPCurrentUnitInfoWidget::InitializeCurrentUnitInfo(ALFPTurnManager* InTurnManager)
{
	if (InTurnManager)
	{
		SetTurnManager(InTurnManager);
	}
	else if (!TurnManagerRef)
	{
		FindTurnManager();
	}

	RefreshFromTurnManager();
}

void ULFPCurrentUnitInfoWidget::SetTurnManager(ALFPTurnManager* InTurnManager)
{
	if (TurnManagerRef == InTurnManager)
	{
		return;
	}

	UnbindFromTurnManager();
	TurnManagerRef = InTurnManager;

	if (TurnManagerRef)
	{
		TurnManagerRef->OnTurnChanged.AddUniqueDynamic(this, &ULFPCurrentUnitInfoWidget::OnTurnChanged);
		TurnManagerRef->OnPhaseChanged.AddUniqueDynamic(this, &ULFPCurrentUnitInfoWidget::OnPhaseChanged);
	}
}

void ULFPCurrentUnitInfoWidget::RefreshFromTurnManager()
{
	if (bIsInspectionMode) return;

	if (!TurnManagerRef)
	{
		FindTurnManager();
	}

	if (!TurnManagerRef)
	{
		BindToUnit(nullptr);
		return;
	}

	if (bHideOutsideActionPhase &&
		TurnManagerRef->GetCurrentPhase() != EBattlePhase::BP_PlayerActionPhase &&
		TurnManagerRef->GetCurrentPhase() != EBattlePhase::BP_EnemyActionPhase)
	{
		BindToUnit(nullptr);
		return;
	}
	if (bHideDuringEnemyTurn && TurnManagerRef->GetCurrentPhase() == EBattlePhase::BP_EnemyActionPhase)
	{
		BindToUnit(nullptr);
		return;
	}

	BindToUnit(BoundUnit);
}

void ULFPCurrentUnitInfoWidget::BindToUnit(ALFPTacticsUnit* Unit)
{
	if (BoundUnit == Unit)
	{
		RefreshUnitInfo();
		SetVisibility(BoundUnit && BoundUnit->IsAlive()
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
		return;
	}

	UnbindFromUnit();
	BoundUnit = Unit;

	if (!BoundUnit)
	{
		ClearUnitInfo();
		OnBoundUnitChanged(nullptr);
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	BoundUnit->OnHealthChangedDelegate.AddUniqueDynamic(this, &ULFPCurrentUnitInfoWidget::OnHealthChanged);
	BoundUnit->OnDeathDelegate.AddUniqueDynamic(this, &ULFPCurrentUnitInfoWidget::OnUnitDeath);
	BoundUnit->OnRuntimeStatsChangedDelegate.AddUniqueDynamic(this, &ULFPCurrentUnitInfoWidget::OnRuntimeStatsChanged);
	if (ULFPBuffComponent* BuffComponent = BoundUnit->GetBuffComponent())
	{
		BuffComponent->OnBuffListChanged.AddUniqueDynamic(this, &ULFPCurrentUnitInfoWidget::OnBuffListChanged);
	}

	RefreshUnitInfo();
	OnBoundUnitChanged(BoundUnit);
	SetVisibility(BoundUnit->IsAlive() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void ULFPCurrentUnitInfoWidget::UnbindFromUnit()
{
	if (BoundUnit)
	{
		BoundUnit->OnHealthChangedDelegate.RemoveDynamic(this, &ULFPCurrentUnitInfoWidget::OnHealthChanged);
		BoundUnit->OnDeathDelegate.RemoveDynamic(this, &ULFPCurrentUnitInfoWidget::OnUnitDeath);
		BoundUnit->OnRuntimeStatsChangedDelegate.RemoveDynamic(this, &ULFPCurrentUnitInfoWidget::OnRuntimeStatsChanged);
		if (ULFPBuffComponent* BuffComponent = BoundUnit->GetBuffComponent())
		{
			BuffComponent->OnBuffListChanged.RemoveDynamic(this, &ULFPCurrentUnitInfoWidget::OnBuffListChanged);
		}
		BoundUnit = nullptr;
	}
}

void ULFPCurrentUnitInfoWidget::SetInspectionMode(bool bInspection)
{
	bIsInspectionMode = bInspection;
	if (InspectionOverlay)
	{
		InspectionOverlay->SetVisibility(bInspection
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Hidden);
	}
}

void ULFPCurrentUnitInfoWidget::RefreshUnitInfo()
{
	if (!BoundUnit)
	{
		ClearUnitInfo();
		return;
	}

	SetOptionalText(UnitNameText, GetUnitDisplayName(BoundUnit));
	SetOptionalText(TierText, FText::AsNumber(BoundUnit->UnitTier));

	if (AffiliationText)
	{
		if (const UEnum* AffiliationEnum = StaticEnum<EUnitAffiliation>())
		{
			AffiliationText->SetText(AffiliationEnum->GetDisplayNameTextByValue(static_cast<int64>(BoundUnit->GetAffiliation())));
		}
	}

	if (UnitIconImage)
	{
		UnitIconImage->SetBrushFromTexture(GetUnitDisplayIcon(BoundUnit));
	}

	RefreshHealth();
	RefreshStats();
	RefreshBuffIcons();
	RefreshCarriedCards();

	OnUnitInfoRefreshed(BoundUnit);
}

void ULFPCurrentUnitInfoWidget::RefreshHealth()
{
	if (!BoundUnit)
	{
		if (HealthProgressBar)
		{
			HealthProgressBar->SetPercent(0.0f);
		}
		SetOptionalText(HealthText, FText::GetEmpty());
		return;
	}

	const int32 CurrentHealth = BoundUnit->GetCurrentHealth();
	const int32 MaxHealth = BoundUnit->GetMaxHealth();
	const float HealthPercent = MaxHealth > 0
		? static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth)
		: 0.0f;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(HealthPercent);
	}

	SetOptionalText(HealthText, FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentHealth, MaxHealth)));
}

void ULFPCurrentUnitInfoWidget::RefreshStats()
{
	if (!BoundUnit)
	{
		SetOptionalText(AttackText, FText::GetEmpty());
		ResetStatTextColor(AttackText);
		SetOptionalText(AttackTypeText, FText::GetEmpty());
		SetOptionalText(MoveText, FText::GetEmpty());
		ResetStatTextColor(MoveText);
		SetOptionalText(SpeedText, FText::GetEmpty());
		ResetStatTextColor(SpeedText);
		SetOptionalText(AttackCountText, FText::GetEmpty());
		ResetStatTextColor(AttackCountText);
		SetOptionalText(ActionCountText, FText::GetEmpty());
		ResetStatTextColor(ActionCountText);
		SetOptionalText(PhysicalBlockText, FText::GetEmpty());
		ResetStatTextColor(PhysicalBlockText);
		SetOptionalText(SpellDefenseText, FText::GetEmpty());
		ResetStatTextColor(SpellDefenseText);
		SetOptionalText(WeightText, FText::GetEmpty());
		ResetStatTextColor(WeightText);
		return;
	}

	SetStatTextWithBaseComparison(
		AttackText,
		FText::AsNumber(BoundUnit->GetCurrentAttack()),
		BoundUnit->GetCurrentAttack(),
		BoundUnit->GetBaseAttack());
	SetStatTextWithBaseComparison(
		MoveText,
		FText::FromString(FString::Printf(
			TEXT("%d / %d"),
			BoundUnit->GetCurrentMovePoints(),
			BoundUnit->GetCurrentMaxMovePoints())),
		BoundUnit->GetCurrentMaxMovePoints(),
		BoundUnit->GetBaseMovePoints());
	SetStatTextWithBaseComparison(
		SpeedText,
		FText::AsNumber(BoundUnit->GetCurrentSpeed()),
		BoundUnit->GetCurrentSpeed(),
		BoundUnit->GetBaseSpeed());
	SetStatTextWithBaseComparison(
		AttackCountText,
		FText::AsNumber(BoundUnit->GetAttackCount()),
		BoundUnit->GetAttackCount(),
		BoundUnit->GetBaseAttackCount());
	SetStatTextWithBaseComparison(
		ActionCountText,
		FText::AsNumber(BoundUnit->GetActionCount()),
		BoundUnit->GetActionCount(),
		BoundUnit->GetBaseActionCount());
	SetStatTextWithBaseComparison(
		PhysicalBlockText,
		FText::AsNumber(BoundUnit->GetPhysicalBlock()),
		BoundUnit->GetPhysicalBlock(),
		BoundUnit->GetBasePhysicalBlock());
	SetStatTextWithBaseComparison(
		SpellDefenseText,
		FText::AsNumber(BoundUnit->GetSpellDefense()),
		BoundUnit->GetSpellDefense(),
		BoundUnit->GetBaseSpellDefense());
	SetStatTextWithBaseComparison(
		WeightText,
		FText::AsNumber(BoundUnit->GetWeight()),
		BoundUnit->GetWeight(),
		BoundUnit->GetBaseWeight());

	if (AttackTypeText)
	{
		if (const UEnum* AttackTypeEnum = StaticEnum<ELFPAttackType>())
		{
			AttackTypeText->SetText(AttackTypeEnum->GetDisplayNameTextByValue(static_cast<int64>(BoundUnit->GetAttackType())));
		}
	}
}

void ULFPCurrentUnitInfoWidget::RefreshBuffIcons()
{
	if (!BuffContainer)
	{
		return;
	}

	BuffContainer->ClearChildren();

	if (!BoundUnit || !BuffIconWidgetClass || MaxBuffIcons <= 0)
	{
		return;
	}

	ULFPBuffComponent* BuffComponent = BoundUnit->GetBuffComponent();
	if (!BuffComponent)
	{
		return;
	}

	const TArray<FLFPBuffDisplayEntry> BuffEntries = BuffComponent->GetAggregatedVisibleBuffDisplayEntries();
	const int32 DisplayCount = FMath::Min(BuffEntries.Num(), MaxBuffIcons);
	for (int32 Index = 0; Index < DisplayCount; ++Index)
	{
		ULFPBuffIconWidget* BuffIconWidget = CreateWidget<ULFPBuffIconWidget>(this, BuffIconWidgetClass);
		if (!BuffIconWidget)
		{
			continue;
		}

		BuffIconWidget->SetBuffEntry(BuffEntries[Index]);
		BuffContainer->AddChild(BuffIconWidget);
	}
}

void ULFPCurrentUnitInfoWidget::RefreshCarriedCards()
{
	DisplayedCarriedCards.Reset();

	if (CarriedCardContainer)
	{
		CarriedCardContainer->ClearChildren();
	}

	if (CarriedCardCountText)
	{
		CarriedCardCountText->SetText(FText::GetEmpty());
	}

	if (!BoundUnit || !CarriedCardContainer || !CarriedCardItemWidgetClass || MaxCarriedCards <= 0)
	{
		return;
	}

	BuildCarriedCardInstances(DisplayedCarriedCards);

	if (CarriedCardCountText)
	{
		CarriedCardCountText->SetText(FText::AsNumber(DisplayedCarriedCards.Num()));
	}

	ALFPTacticsPlayerController* TacticsPC = Cast<ALFPTacticsPlayerController>(GetOwningPlayer());
	for (const FLFPCardInstance& Card : DisplayedCarriedCards)
	{
		if (!Card.IsValid())
		{
			continue;
		}

		ULFPCardItemWidget* CardItem = CreateWidget<ULFPCardItemWidget>(this, CarriedCardItemWidgetClass);
		if (!CardItem)
		{
			continue;
		}

		CardItem->InitializeCardItem(Card, TacticsPC);
		CarriedCardContainer->AddChild(CardItem);
	}
}

void ULFPCurrentUnitInfoWidget::OnTurnChanged()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
		World->GetTimerManager().SetTimerForNextTick(this, &ULFPCurrentUnitInfoWidget::RefreshFromTurnManager);
	}
}

void ULFPCurrentUnitInfoWidget::OnPhaseChanged(EBattlePhase NewPhase)
{
	RefreshFromTurnManager();
}

void ULFPCurrentUnitInfoWidget::OnHealthChanged(int32 CurrentHealth, int32 MaxHealth)
{
	RefreshHealth();
}

void ULFPCurrentUnitInfoWidget::OnUnitDeath()
{
	BindToUnit(nullptr);
}

void ULFPCurrentUnitInfoWidget::OnBuffListChanged()
{
	RefreshBuffIcons();
}

void ULFPCurrentUnitInfoWidget::OnRuntimeStatsChanged(ALFPTacticsUnit* Unit)
{
	if (Unit == BoundUnit)
	{
		RefreshHealth();
		RefreshStats();
	}
}

void ULFPCurrentUnitInfoWidget::FindTurnManager()
{
	if (!GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTurnManager::StaticClass(), FoundManagers);
	if (FoundManagers.Num() > 0)
	{
		SetTurnManager(Cast<ALFPTurnManager>(FoundManagers[0]));
	}
}

void ULFPCurrentUnitInfoWidget::ClearUnitInfo()
{
	SetOptionalText(UnitNameText, FText::GetEmpty());
	SetOptionalText(TierText, FText::GetEmpty());
	SetOptionalText(AffiliationText, FText::GetEmpty());
	RefreshHealth();
	RefreshStats();

	if (UnitIconImage)
	{
		UnitIconImage->SetBrushFromTexture(nullptr);
	}

	if (BuffContainer)
	{
		BuffContainer->ClearChildren();
	}

	DisplayedCarriedCards.Reset();
	if (CarriedCardContainer)
	{
		CarriedCardContainer->ClearChildren();
	}
	SetOptionalText(CarriedCardCountText, FText::GetEmpty());
}

void ULFPCurrentUnitInfoWidget::UnbindFromTurnManager()
{
	if (TurnManagerRef)
	{
		TurnManagerRef->OnTurnChanged.RemoveDynamic(this, &ULFPCurrentUnitInfoWidget::OnTurnChanged);
		TurnManagerRef->OnPhaseChanged.RemoveDynamic(this, &ULFPCurrentUnitInfoWidget::OnPhaseChanged);
		TurnManagerRef = nullptr;
	}
}

void ULFPCurrentUnitInfoWidget::SetOptionalText(UTextBlock* TextBlock, const FText& Text) const
{
	if (TextBlock)
	{
		TextBlock->SetText(Text);
	}
}

void ULFPCurrentUnitInfoWidget::CacheDefaultStatTextColors()
{
	DefaultStatTextColors.Empty();

	UTextBlock* StatTextBlocks[] =
	{
		AttackText,
		MoveText,
		SpeedText,
		AttackCountText,
		ActionCountText,
		PhysicalBlockText,
		SpellDefenseText,
		WeightText
	};

	for (UTextBlock* TextBlock : StatTextBlocks)
	{
		if (TextBlock)
		{
			DefaultStatTextColors.Add(TextBlock, TextBlock->GetColorAndOpacity());
		}
	}
}

void ULFPCurrentUnitInfoWidget::SetStatTextWithBaseComparison(UTextBlock* TextBlock, const FText& Text, int32 CurrentValue, int32 BaseValue)
{
	SetOptionalText(TextBlock, Text);

	if (!TextBlock)
	{
		return;
	}

	if (CurrentValue > BaseValue)
	{
		TextBlock->SetColorAndOpacity(FSlateColor(IncreasedStatColor));
	}
	else if (CurrentValue < BaseValue)
	{
		TextBlock->SetColorAndOpacity(FSlateColor(DecreasedStatColor));
	}
	else
	{
		ResetStatTextColor(TextBlock);
	}
}

void ULFPCurrentUnitInfoWidget::ResetStatTextColor(UTextBlock* TextBlock)
{
	if (!TextBlock)
	{
		return;
	}

	if (const FSlateColor* DefaultColor = DefaultStatTextColors.Find(TextBlock))
	{
		TextBlock->SetColorAndOpacity(*DefaultColor);
	}
}

FText ULFPCurrentUnitInfoWidget::GetUnitDisplayName(ALFPTacticsUnit* Unit) const
{
	if (!Unit)
	{
		return FText::GetEmpty();
	}

	if (const ULFPGameInstance* GameInstance = Cast<ULFPGameInstance>(GetGameInstance()))
	{
		FLFPUnitRegistryEntry RegistryEntry;
		if (GameInstance->UnitRegistry &&
			GameInstance->UnitRegistry->FindEntry(Unit->UnitTypeID, RegistryEntry) &&
			!RegistryEntry.DisplayName.IsEmpty())
		{
			return RegistryEntry.DisplayName;
		}
	}

	return Unit->UnitTypeID != NAME_None
		? FText::FromName(Unit->UnitTypeID)
		: FText::FromString(Unit->GetName());
}

UTexture2D* ULFPCurrentUnitInfoWidget::GetUnitDisplayIcon(ALFPTacticsUnit* Unit) const
{
	if (!Unit)
	{
		return nullptr;
	}

	if (UTexture2D* UnitIcon = Unit->GetUnitIcon())
	{
		return UnitIcon;
	}

	if (const ULFPGameInstance* GameInstance = Cast<ULFPGameInstance>(GetGameInstance()))
	{
		FLFPUnitRegistryEntry RegistryEntry;
		if (GameInstance->UnitRegistry &&
			GameInstance->UnitRegistry->FindEntry(Unit->UnitTypeID, RegistryEntry) &&
			RegistryEntry.Icon)
		{
			return RegistryEntry.Icon;
		}
	}

	return nullptr;
}

void ULFPCurrentUnitInfoWidget::BuildCarriedCardInstances(TArray<FLFPCardInstance>& OutCards)
{
	OutCards.Reset();

	if (!BoundUnit)
	{
		return;
	}

	ULFPGameInstance* GameInstance = Cast<ULFPGameInstance>(GetGameInstance());
	if (ALFPTacticsPlayerController* TacticsPC = Cast<ALFPTacticsPlayerController>(GetOwningPlayer()))
	{
		if (ULFPBattleCardComponent* CardComponent = TacticsPC->GetBattleCardComponent())
		{
			OutCards = CardComponent->BuildUnitCardsPreview(GameInstance, BoundUnit);
			if (OutCards.Num() > MaxCarriedCards)
			{
				OutCards.SetNum(MaxCarriedCards);
			}
			return;
		}
	}

	if (!GameInstance || !GameInstance->UnitRegistry)
	{
		return;
	}

	FLFPUnitRegistryEntry RegistryEntry;
	if (!GameInstance->UnitRegistry->FindEntry(BoundUnit->UnitTypeID, RegistryEntry))
	{
		return;
	}

	if (!RegistryEntry.DefaultCarriedCards.IsEmpty())
	{
		for (const TSoftObjectPtr<ULFPCardDataAsset>& CardData : RegistryEntry.DefaultCarriedCards)
		{
			AddCarriedCardData(CardData, OutCards);
			if (OutCards.Num() >= MaxCarriedCards)
			{
				break;
			}
		}
		return;
	}

	for (TSubclassOf<ULFPSkillBase> SkillClass : RegistryEntry.DefaultCarriedCardSkillClasses)
	{
		AddCarriedCardSkillClass(SkillClass, OutCards);
		if (OutCards.Num() >= MaxCarriedCards)
		{
			break;
		}
	}
}

bool ULFPCurrentUnitInfoWidget::AddCarriedCardData(
	const TSoftObjectPtr<ULFPCardDataAsset>& CardData,
	TArray<FLFPCardInstance>& OutCards)
{
	if (OutCards.Num() >= MaxCarriedCards)
	{
		return false;
	}

	ULFPCardDataAsset* LoadedCardData = CardData.LoadSynchronous();
	if (!LoadedCardData || !LoadedCardData->IsValidCardData())
	{
		return false;
	}

	return AddCarriedCardDefinition(LoadedCardData->BuildCardDefinition(), OutCards);
}

bool ULFPCurrentUnitInfoWidget::AddCarriedCardSkillClass(
	TSubclassOf<ULFPSkillBase> SkillClass,
	TArray<FLFPCardInstance>& OutCards)
{
	if (!SkillClass || OutCards.Num() >= MaxCarriedCards)
	{
		return false;
	}

	FLFPCardDefinition Definition;
	Definition.SkillClass = SkillClass;
	Definition.DestinationAfterPlay = ELFPCardPile::DiscardPile;
	Definition.CardCategory = ELFPCardCategory::RaceSpecific;
	return AddCarriedCardDefinition(Definition, OutCards);
}

bool ULFPCurrentUnitInfoWidget::AddCarriedCardDefinition(
	const FLFPCardDefinition& Definition,
	TArray<FLFPCardInstance>& OutCards)
{
	if (!Definition.SkillClass || OutCards.Num() >= MaxCarriedCards)
	{
		return false;
	}

	ULFPSkillBase* RuntimeSkill = NewObject<ULFPSkillBase>(this, Definition.SkillClass);
	if (!RuntimeSkill)
	{
		return false;
	}

	RuntimeSkill->InitSkill(BoundUnit);

	FLFPCardInstance Card;
	Card.InstanceID = OutCards.Num() + 1;
	Card.SourceUnit = BoundUnit;
	Card.RuntimeSkill = RuntimeSkill;
	Card.CurrentPile = ELFPCardPile::DrawPile;
	Card.Definition = Definition;

	if (Card.Definition.CardID.IsNone())
	{
		Card.Definition.CardID = FName(*FString::Printf(TEXT("%s_%d"),
			*Definition.SkillClass->GetName(),
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
