// Fill out your copyright notice in the Description page of Project Settings.

#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPTurnSpeedListWidget::SetRoundNumber(int32 Round)
{
	if (RoundText)
	{
		RoundText->SetText(FText::AsNumber(Round));
	}
}
void ULFPTurnSpeedListWidget::UpdateTurnOrder(const TArray<ALFPTacticsUnit*>& Units, int32 CurrentUnitIndex)
{
	//if (!UnitIconsContainer || !UnitIconClass) return;
	//// 清空现有图标
	//UnitIconsContainer->ClearChildren();
	//// 为每个单位添加图标
	//for (int32 i = 0; i < Units.Num(); i++)
	//{
	//	ALFPTacticsUnit* Unit = Units[i];
	//	if (Unit)
	//	{
	//		UUserWidget* IconWidget = CreateWidget<UUserWidget>(this, UnitIconClass);
	//		if (IconWidget)
	//		{
	//			// 设置图标（这里假设在UnitIconClass中有一个Image控件名为"UnitIcon"，并且有一个函数或公开属性设置高亮）
	//			UImage* IconImage = Cast<UImage>(IconWidget->GetWidgetFromName(TEXT("UnitIcon")));
	//			if (IconImage)
	//			{
	//				// 设置图片，这里需要单位有头像资源，可以在单位类中获取
	//				// 例如：IconImage->SetBrushFromTexture(Unit->GetUnitIcon());
	//				// 暂时用占位，具体需要您在单位类中添加获取头像的函数
	//			}
	//			// 高亮当前单位
	//			UWidget* Highlight = IconWidget->GetWidgetFromName(TEXT("Highlight"));
	//			if (Highlight)
	//			{
	//				Highlight->SetVisibility(i == CurrentUnitIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	//			}
	//			// 添加到水平框
	//			UHorizontalBoxSlot* Slot = UnitIconsContainer->AddChildToHorizontalBox(IconWidget);
	//			Slot->SetPadding(FMargin(5, 0, 5, 0)); // 设置间距
	//			Slot->SetHorizontalAlignment(HAlign_Center);
	//			Slot->SetVerticalAlignment(VAlign_Center);
	//		}
	//	}
	//}
}