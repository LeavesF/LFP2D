#include "LFP2D/UI/Fighting/LFPBattleResultWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"

void ULFPBattleResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddDynamic(this, &ULFPBattleResultWidget::OnConfirmClicked);
	}
}

void ULFPBattleResultWidget::Setup(const FLFPBattleResult& Result, ULFPUnitRegistryDataAsset* Registry)
{
	CachedResult = Result;
	UnitRegistry = Registry;

	// 设置胜负文本
	if (Text_Result)
	{
		if (Result.bVictory)
		{
			Text_Result->SetText(FText::FromString(TEXT("胜利")));
		}
		else if (Result.bEscaped)
		{
			Text_Result->SetText(FText::FromString(TEXT("撤退")));
		}
		else
		{
			Text_Result->SetText(FText::FromString(TEXT("败北")));
		}
	}

	// 胜利：显示奖励
	if (Result.bVictory)
	{
		if (Text_Gold)
		{
			Text_Gold->SetText(FText::FromString(FString::Printf(TEXT("金币: +%d"), Result.GoldReward)));
		}
		if (Text_Food)
		{
			Text_Food->SetText(FText::FromString(FString::Printf(TEXT("食物: +%d"), Result.FoodReward)));
		}

		// 捕获单位列表
		if (Box_CapturedUnits)
		{
			if (Result.CapturedUnits.Num() == 0)
			{
				Box_CapturedUnits->SetVisibility(ESlateVisibility::Collapsed);
			}
			// 捕获单位的具体图标展示可在蓝图中扩展
		}
	}
	else
	{
		// 失败/逃跑：隐藏奖励区域
		if (Text_Gold)
		{
			Text_Gold->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Text_Food)
		{
			Text_Food->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Box_CapturedUnits)
		{
			Box_CapturedUnits->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ULFPBattleResultWidget::OnConfirmClicked()
{
	OnConfirmPressed.Broadcast();
}
