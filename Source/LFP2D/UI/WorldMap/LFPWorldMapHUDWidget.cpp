#include "LFP2D/UI/WorldMap/LFPWorldMapHUDWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/WorldMap/LFPWorldMapPlayerState.h"
#include "LFP2D/Shop/LFPRelicTypes.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"

void ULFPWorldMapHUDWidget::Setup(ULFPGameInstance* GI, ULFPWorldMapPlayerState* PS)
{
	// 先解绑旧委托（防止重复绑定）
	UnbindDelegates();

	CachedGI = GI;
	CachedPS = PS;

	// 绑定 GameInstance 委托
	if (CachedGI)
	{
		CachedGI->OnResourceChanged.AddDynamic(this, &ULFPWorldMapHUDWidget::OnResourceChanged);
		CachedGI->OnOwnedRelicsChanged.AddDynamic(this, &ULFPWorldMapHUDWidget::OnOwnedRelicsChanged);
	}

	// 绑定 PlayerState 委托
	if (CachedPS)
	{
		CachedPS->OnTurnChanged.AddDynamic(this, &ULFPWorldMapHUDWidget::OnTurnChanged);
	}

	// 初始刷新（拉取当前值）
	RefreshAll();
}

void ULFPWorldMapHUDWidget::NativeDestruct()
{
	UnbindDelegates();
	Super::NativeDestruct();
}

void ULFPWorldMapHUDWidget::UnbindDelegates()
{
	if (CachedGI)
	{
		CachedGI->OnResourceChanged.RemoveDynamic(this, &ULFPWorldMapHUDWidget::OnResourceChanged);
		CachedGI->OnOwnedRelicsChanged.RemoveDynamic(this, &ULFPWorldMapHUDWidget::OnOwnedRelicsChanged);
	}

	if (CachedPS)
	{
		CachedPS->OnTurnChanged.RemoveDynamic(this, &ULFPWorldMapHUDWidget::OnTurnChanged);
	}
}

// ============== 刷新方法 ==============

void ULFPWorldMapHUDWidget::RefreshAll()
{
	RefreshResources();
	RefreshTurnInfo();
	RefreshRelicList();
}

void ULFPWorldMapHUDWidget::RefreshResources()
{
	if (!CachedGI) return;

	if (Text_Gold)
	{
		Text_Gold->SetText(FText::FromString(
			FString::Printf(TEXT("%d"), CachedGI->GetGold())));
	}

	if (Text_Food)
	{
		Text_Food->SetText(FText::FromString(
			FString::Printf(TEXT("%d"), CachedGI->GetFood())));
	}
}

void ULFPWorldMapHUDWidget::RefreshTurnInfo()
{
	if (!CachedPS) return;

	if (Text_Turn)
	{
		Text_Turn->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), CachedPS->CurrentTurn, CachedPS->TurnPressureThreshold)));
	}
}

void ULFPWorldMapHUDWidget::RefreshRelicList()
{
	if (!Box_RelicList || !CachedGI) return;

	Box_RelicList->ClearChildren();

	TArray<FName> RelicIDs = CachedGI->GetOwnedRelicIDsArray();

	for (const FName& RelicID : RelicIDs)
	{
		FLFPRelicDefinition Definition;
		if (!CachedGI->FindRelicDefinition(RelicID, Definition))
		{
			continue;
		}

		// 创建遗物图标
		UImage* RelicIcon = NewObject<UImage>(this);
		if (!RelicIcon) continue;

		if (Definition.Icon)
		{
			RelicIcon->SetBrushFromTexture(Definition.Icon);
		}

		RelicIcon->SetDesiredSizeOverride(FVector2D(48.f, 48.f));

		// 设置 Tooltip：遗物名称 + 描述
		FString TooltipStr = Definition.DisplayName.ToString();
		if (!Definition.Description.IsEmpty())
		{
			TooltipStr += TEXT("\n") + Definition.Description.ToString();
		}
		RelicIcon->SetToolTipText(FText::FromString(TooltipStr));

		Box_RelicList->AddChild(RelicIcon);

		RelicIcon->SetDesiredSizeOverride(FVector2D(RelicIconSize, RelicIconSize));
	}
}

// ============== 委托回调 ==============

void ULFPWorldMapHUDWidget::OnResourceChanged(int32 NewGold, int32 NewFood)
{
	RefreshResources();
}

void ULFPWorldMapHUDWidget::OnOwnedRelicsChanged()
{
	RefreshRelicList();
}

void ULFPWorldMapHUDWidget::OnTurnChanged(int32 CurrentTurn, int32 TurnBudget)
{
	RefreshTurnInfo();
}
