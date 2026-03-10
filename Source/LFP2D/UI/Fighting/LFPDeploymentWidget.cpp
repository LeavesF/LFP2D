#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Components/Button.h"
#include "Components/Image.h"

void ULFPDeploymentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定按钮点击事件
	if (Button_Unit0) Button_Unit0->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnUnit0Clicked);
	if (Button_Unit1) Button_Unit1->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnUnit1Clicked);
	if (Button_Unit2) Button_Unit2->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnUnit2Clicked);
	if (Button_Confirm) Button_Confirm->OnClicked.AddDynamic(this, &ULFPDeploymentWidget::OnConfirmClicked);
}

void ULFPDeploymentWidget::Setup(const TArray<FLFPUnitEntry>& PartyUnits, ULFPUnitRegistryDataAsset* Registry)
{
	CachedPartyUnits = PartyUnits;
	UnitRegistry = Registry;

	// 初始化放置状态
	PlacedStates.Empty();
	PlacedStates.SetNum(3);
	for (int32 i = 0; i < 3; i++)
	{
		PlacedStates[i] = false;
	}

	bConfirmEnabled = false;

	// 设置各槽位图标和可见性
	for (int32 i = 0; i < 3; i++)
	{
		UButton* Btn = GetUnitButton(i);
		UImage* Img = GetUnitImage(i);

		if (i < PartyUnits.Num() && PartyUnits[i].IsValid() && Registry)
		{
			// 有单位：显示按钮，设置图标
			if (Btn) Btn->SetVisibility(ESlateVisibility::Visible);

			FLFPUnitRegistryEntry Entry;
			if (Img && Registry->FindEntry(PartyUnits[i].TypeID, Entry) && Entry.Icon)
			{
				Img->SetBrushFromTexture(Entry.Icon);
				Img->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else
		{
			// 无单位：隐藏按钮
			if (Btn) Btn->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 确认按钮初始禁用
	if (Button_Confirm)
	{
		Button_Confirm->SetIsEnabled(false);
	}
}

void ULFPDeploymentWidget::MarkUnitPlaced(int32 PartyIndex, bool bPlaced)
{
	if (!PlacedStates.IsValidIndex(PartyIndex)) return;

	PlacedStates[PartyIndex] = bPlaced;
	UpdateSlotVisual(PartyIndex);

	// 检查是否全部放置完毕
	bool bAllPlaced = true;
	for (int32 i = 0; i < CachedPartyUnits.Num() && i < PlacedStates.Num(); i++)
	{
		if (CachedPartyUnits[i].IsValid() && !PlacedStates[i])
		{
			bAllPlaced = false;
			break;
		}
	}
	SetConfirmEnabled(bAllPlaced);
}

void ULFPDeploymentWidget::SetConfirmEnabled(bool bEnabled)
{
	bConfirmEnabled = bEnabled;
	if (Button_Confirm)
	{
		Button_Confirm->SetIsEnabled(bEnabled);
	}
}

void ULFPDeploymentWidget::MarkUnitSelecting(int32 PartyIndex)
{
	// 可扩展：高亮正在放置的槽位
}

void ULFPDeploymentWidget::UpdateSlotVisual(int32 Index)
{
	UImage* Img = GetUnitImage(Index);
	if (!Img) return;

	if (PlacedStates.IsValidIndex(Index) && PlacedStates[Index])
	{
		// 已放置：半透明
		Img->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
	}
	else
	{
		// 未放置：正常
		Img->SetColorAndOpacity(FLinearColor::White);
	}
}

// ============== 按钮回调 ==============

void ULFPDeploymentWidget::OnUnit0Clicked()
{
	OnUnitSelected.Broadcast(0);
}

void ULFPDeploymentWidget::OnUnit1Clicked()
{
	OnUnitSelected.Broadcast(1);
}

void ULFPDeploymentWidget::OnUnit2Clicked()
{
	OnUnitSelected.Broadcast(2);
}

void ULFPDeploymentWidget::OnConfirmClicked()
{
	if (bConfirmEnabled)
	{
		OnConfirmPressed.Broadcast();
	}
}

// ============== 辅助方法 ==============

UButton* ULFPDeploymentWidget::GetUnitButton(int32 Index) const
{
	switch (Index)
	{
	case 0: return Button_Unit0;
	case 1: return Button_Unit1;
	case 2: return Button_Unit2;
	default: return nullptr;
	}
}

UImage* ULFPDeploymentWidget::GetUnitImage(int32 Index) const
{
	switch (Index)
	{
	case 0: return Image_Unit0;
	case 1: return Image_Unit1;
	case 2: return Image_Unit2;
	default: return nullptr;
	}
}
