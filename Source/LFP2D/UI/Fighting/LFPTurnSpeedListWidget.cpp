// Fill out your copyright notice in the Description page of Project Settings.

#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPTurnSpeedListWidget::InitializeTurnOrder()
{
	if (!TurnManagerRef)
	{
		// 查找回合管理器
		TArray<AActor*> FoundManagers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTurnManager::StaticClass(), FoundManagers);
		if (FoundManagers.Num() > 0)
		{
			TurnManagerRef = Cast<ALFPTurnManager>(FoundManagers[0]);
		}
	}

	if (TurnManagerRef)
	{
		// 绑定回合变化事件
		TurnManagerRef->OnTurnChanged.AddDynamic(this, &ULFPTurnSpeedListWidget::OnTurnChanged);

		// 立即更新UI
		UpdateTurnOrder();
	}
}

void ULFPTurnSpeedListWidget::SetRoundNumber(int32 Round)
{
	if (RoundText)
	{
		RoundText->SetText(FText::AsNumber(Round));
	}
}
void ULFPTurnSpeedListWidget::UpdateTurnOrder()
{
	if (!TurnManagerRef)
	{
		return;
	}
	// 设置回合文本
	RoundText->SetText(FText::FromString(FString::Printf(TEXT("Round %d"), TurnManagerRef->GetCurrentRound())));

	// 复用现有图标而不是销毁重建
	TArray<UWidget*> ExistingIcons = UnitIconsContainer->GetAllChildren();

	TArray<ALFPTacticsUnit*> TurnOrderUnits = TurnManagerRef->GetTurnOrderUnits();
	
	//for (int32 i = 0; i < TurnOrderUnits.Num(); i++)
	//{
	//	UWBP_UnitTurnIcon* Icon = nullptr;

	//	if (i < ExistingIcons.Num())
	//	{
	//		// 复用现有图标
	//		Icon = Cast<UWBP_UnitTurnIcon>(ExistingIcons[i]);
	//	}
	//	else
	//	{
	//		// 创建新图标
	//		Icon = CreateWidget<UWBP_UnitTurnIcon>(this, UnitIconClass);
	//		UnitIconsContainer->AddChild(Icon);
	//	}

	//	// 更新图标内容
	//	ALFPTacticsUnit* Unit = TurnManagerRef->TurnOrderArray[i];
	//	if (Icon && Unit)
	//	{
	//		Icon->SetUnit(Unit);
	//		Icon->SetIsCurrent(TurnManagerRef->GetCurrentTurnIndex() == i);
	//	}
	//}

	//// 移除多余图标
	//for (int32 i = TurnOrderUnits.Num(); i < ExistingIcons.Num(); i++)
	//{
	//	ExistingIcons[i]->RemoveFromParent();
	//}


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

void ULFPTurnSpeedListWidget::OnTurnChanged()
{
	// 取消之前的延迟更新
	GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);

	// 延迟一帧更新，避免同一帧内多次更新
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ULFPTurnSpeedListWidget::UpdateTurnOrder);
}