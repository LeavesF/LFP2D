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
	//// �������ͼ��
	//UnitIconsContainer->ClearChildren();
	//// Ϊÿ����λ���ͼ��
	//for (int32 i = 0; i < Units.Num(); i++)
	//{
	//	ALFPTacticsUnit* Unit = Units[i];
	//	if (Unit)
	//	{
	//		UUserWidget* IconWidget = CreateWidget<UUserWidget>(this, UnitIconClass);
	//		if (IconWidget)
	//		{
	//			// ����ͼ�꣨���������UnitIconClass����һ��Image�ؼ���Ϊ"UnitIcon"��������һ�������򹫿��������ø�����
	//			UImage* IconImage = Cast<UImage>(IconWidget->GetWidgetFromName(TEXT("UnitIcon")));
	//			if (IconImage)
	//			{
	//				// ����ͼƬ��������Ҫ��λ��ͷ����Դ�������ڵ�λ���л�ȡ
	//				// ���磺IconImage->SetBrushFromTexture(Unit->GetUnitIcon());
	//				// ��ʱ��ռλ��������Ҫ���ڵ�λ������ӻ�ȡͷ��ĺ���
	//			}
	//			// ������ǰ��λ
	//			UWidget* Highlight = IconWidget->GetWidgetFromName(TEXT("Highlight"));
	//			if (Highlight)
	//			{
	//				Highlight->SetVisibility(i == CurrentUnitIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	//			}
	//			// ��ӵ�ˮƽ��
	//			UHorizontalBoxSlot* Slot = UnitIconsContainer->AddChildToHorizontalBox(IconWidget);
	//			Slot->SetPadding(FMargin(5, 0, 5, 0)); // ���ü��
	//			Slot->SetHorizontalAlignment(HAlign_Center);
	//			Slot->SetVerticalAlignment(VAlign_Center);
	//		}
	//	}
	//}
}