#include "LFP2D/UI/Fighting/LFPCurrentUnitInfoWidget.h"

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
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "TimerManager.h"

void ULFPCurrentUnitInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
	if (!TurnManagerRef)
	{
		FindTurnManager();
	}

	if (!TurnManagerRef)
	{
		BindToUnit(nullptr);
		return;
	}

	ALFPTacticsUnit* CurrentUnit = TurnManagerRef->GetCurrentUnit();
	if (bHideOutsideActionPhase && TurnManagerRef->GetCurrentPhase() != EBattlePhase::BP_ActionPhase)
	{
		CurrentUnit = nullptr;
	}
	if (bHideDuringEnemyTurn && CurrentUnit && CurrentUnit->IsEnemy())
	{
		CurrentUnit = nullptr;
	}

	BindToUnit(CurrentUnit);
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
		if (ULFPBuffComponent* BuffComponent = BoundUnit->GetBuffComponent())
		{
			BuffComponent->OnBuffListChanged.RemoveDynamic(this, &ULFPCurrentUnitInfoWidget::OnBuffListChanged);
		}
		BoundUnit = nullptr;
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
		SetOptionalText(AttackTypeText, FText::GetEmpty());
		SetOptionalText(MoveText, FText::GetEmpty());
		SetOptionalText(SpeedText, FText::GetEmpty());
		SetOptionalText(AttackCountText, FText::GetEmpty());
		SetOptionalText(ActionCountText, FText::GetEmpty());
		SetOptionalText(PhysicalBlockText, FText::GetEmpty());
		SetOptionalText(SpellDefenseText, FText::GetEmpty());
		SetOptionalText(WeightText, FText::GetEmpty());
		return;
	}

	SetOptionalText(AttackText, FText::AsNumber(BoundUnit->GetCurrentAttack()));
	SetOptionalText(MoveText, FText::FromString(FString::Printf(
		TEXT("%d / %d"),
		BoundUnit->GetCurrentMovePoints(),
		BoundUnit->GetCurrentMaxMovePoints())));
	SetOptionalText(SpeedText, FText::AsNumber(BoundUnit->GetCurrentSpeed()));
	SetOptionalText(AttackCountText, FText::AsNumber(BoundUnit->GetAttackCount()));
	SetOptionalText(ActionCountText, FText::AsNumber(BoundUnit->GetActionCount()));
	SetOptionalText(PhysicalBlockText, FText::AsNumber(BoundUnit->GetPhysicalBlock()));
	SetOptionalText(SpellDefenseText, FText::AsNumber(BoundUnit->GetSpellDefense()));
	SetOptionalText(WeightText, FText::AsNumber(BoundUnit->GetWeight()));

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
	RefreshStats();
	RefreshBuffIcons();
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
