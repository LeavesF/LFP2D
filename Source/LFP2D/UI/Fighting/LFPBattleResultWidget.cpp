#include "LFP2D/UI/Fighting/LFPBattleResultWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBoxSlot.h"

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
			Box_CapturedUnits->ClearChildren();

			if (Result.CapturedUnits.Num() == 0)
			{
				Box_CapturedUnits->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				Box_CapturedUnits->SetVisibility(ESlateVisibility::Visible);

				for (const FLFPUnitEntry& CapturedUnit : Result.CapturedUnits)
				{
					FLFPUnitRegistryEntry RegistryEntry;
					if (!UnitRegistry || !UnitRegistry->FindEntry(CapturedUnit.TypeID, RegistryEntry))
					{
						continue;
					}

					// 外层竖排容器：图标在上，名字在下
					UVerticalBox* ItemBox = NewObject<UVerticalBox>(this);

					// 图标
					UImage* IconImage = NewObject<UImage>(this);
					if (RegistryEntry.Icon)
					{
						IconImage->SetBrushFromTexture(RegistryEntry.Icon);
					}
					IconImage->SetDesiredSizeOverride(FVector2D(48.0, 48.0));
					UVerticalBoxSlot* IconSlot = ItemBox->AddChildToVerticalBox(IconImage);
					IconSlot->SetHorizontalAlignment(HAlign_Center);

					// 名称
					UTextBlock* NameText = NewObject<UTextBlock>(this);
					NameText->SetText(RegistryEntry.DisplayName);
					FSlateFontInfo FontInfo = NameText->GetFont();
					FontInfo.Size = 12;
					NameText->SetFont(FontInfo);
					UVerticalBoxSlot* TextSlot = ItemBox->AddChildToVerticalBox(NameText);
					TextSlot->SetHorizontalAlignment(HAlign_Center);

					// 添加到 WrapBox
					UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(Box_CapturedUnits->AddChild(ItemBox));
					if (WrapSlot)
					{
						WrapSlot->SetPadding(FMargin(8.0f, 4.0f));
					}
				}
			}
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
