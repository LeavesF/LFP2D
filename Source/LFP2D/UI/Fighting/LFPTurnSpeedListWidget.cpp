// Fill out your copyright notice in the Description page of Project Settings.

#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedIconWidget.h"
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

		// 绑定阶段变化事件
		TurnManagerRef->OnPhaseChanged.AddDynamic(this, &ULFPTurnSpeedListWidget::OnPhaseChanged);

		// 初始更新UI
		OnPhaseChanged(TurnManagerRef->GetCurrentPhase());
		UpdateTurnOrder();
	}
}

void ULFPTurnSpeedListWidget::UpdateTurnOrder()
{
	if (!TurnManagerRef)
	{
		return;
	}

	TArray<UWidget*> ExistingIcons = UnitIconsContainer->GetAllChildren();

	TArray<ALFPTacticsUnit*> TurnOrderUnits = TurnManagerRef->GetTurnOrderUnits();
	
	for (int32 i = 0; i < TurnOrderUnits.Num(); i++)
	{
		ULFPTurnSpeedIconWidget* Icon = nullptr;

		if (i < ExistingIcons.Num())
		{
			// 复用已有头像
			Icon = Cast<ULFPTurnSpeedIconWidget>(ExistingIcons[i]);
		}
		else
		{
			// 创建新的头像
			Icon = CreateWidget<ULFPTurnSpeedIconWidget>(this, UnitIconClass);
			UnitIconsContainer->AddChild(Icon);
		}

		// 更新头像状态
		ALFPTacticsUnit* Unit = TurnOrderUnits[i];
		if (Icon && Unit)
		{
			Icon->SetUnit(Unit);
			Icon->SetIsCurrent(TurnManagerRef->GetCurrentUnit() == Unit);
			Icon->SetHasActed(Unit->HasActed());
		}
	}

	// 移除多余头像
	for (int32 i = TurnOrderUnits.Num(); i < ExistingIcons.Num(); i++)
	{
		ExistingIcons[i]->RemoveFromParent();
	}

	//if (!UnitIconsContainer || !UnitIconClass) return;
	//// 清空现有头像
	//UnitIconsContainer->ClearChildren();
	//// 为每个单位创建头像
	//for (int32 i = 0; i < Units.Num(); i++)
	//{
	//	ALFPTacticsUnit* Unit = Units[i];
	//	if (Unit)
	//	{
	//		UUserWidget* IconWidget = CreateWidget<UUserWidget>(this, UnitIconClass);
	//		if (IconWidget)
	//		{
	//			// 设置头像控件（假设 UnitIconClass 中有一个名为 "UnitIcon" 的 Image 控件）
	//			UImage* IconImage = Cast<UImage>(IconWidget->GetWidgetFromName(TEXT("UnitIcon")));
	//			if (IconImage)
	//			{
	//				// 设置头像图片，这里需要单位头像资源，可从单位数据中获取
	//				// 例如：IconImage->SetBrushFromTexture(Unit->GetUnitIcon());
	//				// 暂时使用占位逻辑，后续可在单位类中添加获取头像的函数
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

void ULFPTurnSpeedListWidget::OnPhaseChanged(EBattlePhase NewPhase)
{
	SetVisibility(NewPhase == EBattlePhase::BP_Deployment
		? ESlateVisibility::Collapsed
		: ESlateVisibility::SelfHitTestInvisible);
}
