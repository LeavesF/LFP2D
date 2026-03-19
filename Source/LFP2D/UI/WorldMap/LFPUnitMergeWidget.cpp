#include "LFP2D/UI/WorldMap/LFPUnitMergeWidget.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/Texture2D.h"

void ULFPUnitMergeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_SlotA) Button_SlotA->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnSlotAClicked);
	if (Button_SlotB) Button_SlotB->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnSlotBClicked);
	if (Button_Merge) Button_Merge->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnMergeClicked);
	if (Button_Clear) Button_Clear->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnClearClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnCloseClicked);

	if (Button_Merge) Button_Merge->SetIsEnabled(false);
}

void ULFPUnitMergeWidget::Setup(ULFPGameInstance* GI, ULFPUnitRegistryDataAsset* Registry)
{
	CachedGameInstance = GI;
	CachedRegistry = Registry;
	ClearSlots();
	RefreshUnitIcons();
}

// 创建单个单位 icon 按钮的辅助函数
static UButton* CreateUnitIconButton(ULFPUnitMergeWidget* Outer, UHorizontalBox* Container,
	ULFPUnitRegistryDataAsset* Registry, const FLFPUnitEntry& Unit, bool bDisabled)
{
	UButton* Btn = NewObject<UButton>(Outer);
	if (!Btn || !Container) return nullptr;

	UHorizontalBoxSlot* BoxSlot = Container->AddChildToHorizontalBox(Btn);
	if (BoxSlot)
	{
		BoxSlot->SetPadding(FMargin(4.f));
	}

	// 设置图标样式
	FLFPUnitRegistryEntry RegEntry;
	if (Registry && Registry->FindEntry(Unit.TypeID, RegEntry) && RegEntry.Icon)
	{
		FButtonStyle Style = Btn->GetStyle();
		Style.Normal.SetResourceObject(RegEntry.Icon);
		Style.Normal.ImageSize = FVector2D(64.f, 64.f);
		Style.Hovered.SetResourceObject(RegEntry.Icon);
		Style.Hovered.ImageSize = FVector2D(64.f, 64.f);
		Style.Pressed.SetResourceObject(RegEntry.Icon);
		Style.Pressed.ImageSize = FVector2D(64.f, 64.f);
		Btn->SetStyle(Style);
	}

	if (bDisabled)
	{
		Btn->SetIsEnabled(false);
		Btn->SetRenderOpacity(0.4f);
	}

	return Btn;
}

void ULFPUnitMergeWidget::RefreshUnitIcons()
{
	UnitIconButtons.Empty();
	ButtonToUnitMap.Empty();

	if (!CachedGameInstance || !CachedRegistry) return;

	if (Box_PartyUnits) Box_PartyUnits->ClearChildren();
	if (Box_ReserveUnits) Box_ReserveUnits->ClearChildren();

	// 队伍单位
	for (int32 i = 0; i < CachedGameInstance->PartyUnits.Num(); ++i)
	{
		const FLFPUnitEntry& Unit = CachedGameInstance->PartyUnits[i];
		if (!Unit.IsValid()) continue;

		bool bInSlot = (!SlotA.bIsEmpty && SlotA.bIsParty && SlotA.SlotIndex == i)
			|| (!SlotB.bIsEmpty && SlotB.bIsParty && SlotB.SlotIndex == i);

		UButton* Btn = CreateUnitIconButton(this, Box_PartyUnits, CachedRegistry, Unit, bInSlot);
		if (!Btn) continue;

		UnitIconButtons.Add(Btn);
		ButtonToUnitMap.Add(Btn, {true, i});

		if (!bInSlot)
		{
			Btn->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnUnitIconClicked);
		}
	}

	// 备战营单位
	for (int32 i = 0; i < CachedGameInstance->ReserveUnits.Num(); ++i)
	{
		const FLFPUnitEntry& Unit = CachedGameInstance->ReserveUnits[i];
		if (!Unit.IsValid()) continue;

		bool bInSlot = (!SlotA.bIsEmpty && !SlotA.bIsParty && SlotA.SlotIndex == i)
			|| (!SlotB.bIsEmpty && !SlotB.bIsParty && SlotB.SlotIndex == i);

		UButton* Btn = CreateUnitIconButton(this, Box_ReserveUnits, CachedRegistry, Unit, bInSlot);
		if (!Btn) continue;

		UnitIconButtons.Add(Btn);
		ButtonToUnitMap.Add(Btn, {false, i});

		if (!bInSlot)
		{
			Btn->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnUnitIconClicked);
		}
	}
}

void ULFPUnitMergeWidget::OnUnitIconClicked()
{
	// 找到触发点击的按钮
	for (const auto& Pair : ButtonToUnitMap)
	{
		UButton* Btn = Pair.Key;
		if (Btn && Btn->IsHovered())
		{
			PlaceUnitInSlot(Pair.Value.bIsParty, Pair.Value.SlotIndex);
			return;
		}
	}
}

void ULFPUnitMergeWidget::PlaceUnitInSlot(bool bIsParty, int32 Index)
{
	if (!CachedGameInstance) return;

	const TArray<FLFPUnitEntry>& SourceArray = bIsParty
		? CachedGameInstance->PartyUnits
		: CachedGameInstance->ReserveUnits;

	if (!SourceArray.IsValidIndex(Index)) return;

	FLFPMergeSlotInfo NewSlot;
	NewSlot.bIsParty = bIsParty;
	NewSlot.SlotIndex = Index;
	NewSlot.Unit = SourceArray[Index];
	NewSlot.bIsEmpty = false;

	// 放入第一个空的合成框
	if (SlotA.bIsEmpty)
	{
		SlotA = NewSlot;
	}
	else if (SlotB.bIsEmpty)
	{
		SlotB = NewSlot;
	}
	else
	{
		// 两个都满了，不操作
		return;
	}

	UpdateSlotVisual(SlotA, Image_SlotA);
	UpdateSlotVisual(SlotB, Image_SlotB);
	UpdatePreview();
	RefreshUnitIcons();
}

void ULFPUnitMergeWidget::OnSlotAClicked()
{
	if (!SlotA.bIsEmpty)
	{
		SlotA = FLFPMergeSlotInfo();
		UpdateSlotVisual(SlotA, Image_SlotA);
		UpdatePreview();
		RefreshUnitIcons();
	}
}

void ULFPUnitMergeWidget::OnSlotBClicked()
{
	if (!SlotB.bIsEmpty)
	{
		SlotB = FLFPMergeSlotInfo();
		UpdateSlotVisual(SlotB, Image_SlotB);
		UpdatePreview();
		RefreshUnitIcons();
	}
}

void ULFPUnitMergeWidget::ClearSlots()
{
	SlotA = FLFPMergeSlotInfo();
	SlotB = FLFPMergeSlotInfo();
	UpdateSlotVisual(SlotA, Image_SlotA);
	UpdateSlotVisual(SlotB, Image_SlotB);
	UpdatePreview();
}

void ULFPUnitMergeWidget::UpdatePreview()
{
	bool bCanMerge = false;
	SelectedEvolutionTarget = NAME_None;

	// 清空进化目标展示
	if (Box_EvolutionChoices) Box_EvolutionChoices->ClearChildren();
	EvolutionChoiceButtons.Empty();
	EvolutionButtonToTargetMap.Empty();

	if (!SlotA.bIsEmpty && !SlotB.bIsEmpty && CachedRegistry)
	{
		if (SlotA.Unit.TypeID == SlotB.Unit.TypeID)
		{
			TArray<FName> Targets = CachedRegistry->GetEvolutionTargets(SlotA.Unit.TypeID);

			if (Targets.Num() >= 1)
			{
				// 为每个进化目标创建图标按钮
				for (const FName& TargetID : Targets)
				{
					FLFPUnitRegistryEntry TargetEntry;
					if (!CachedRegistry->FindEntry(TargetID, TargetEntry)) continue;

					UButton* ChoiceBtn = NewObject<UButton>(this);
					if (!ChoiceBtn || !Box_EvolutionChoices) continue;

					EvolutionChoiceButtons.Add(ChoiceBtn);
					EvolutionButtonToTargetMap.Add(ChoiceBtn, TargetID);

					UHorizontalBoxSlot* BoxSlot = Box_EvolutionChoices->AddChildToHorizontalBox(ChoiceBtn);
					if (BoxSlot)
					{
						BoxSlot->SetPadding(FMargin(30.f));
						BoxSlot->SetHorizontalAlignment(HAlign_Center);
						BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
					}

					if (TargetEntry.Icon)
					{
						FButtonStyle Style = ChoiceBtn->GetStyle();
						Style.Hovered.SetResourceObject(TargetEntry.Icon);
						Style.Hovered.ImageSize = FVector2D(220.f, 80.f);

						if (Targets.Num() == 1)
						{
							// 单目标：三态一致，看起来不像按钮
							Style.Normal = Style.Hovered;
							Style.Pressed = Style.Hovered;
						}
						else
						{
							Style.Normal.SetResourceObject(TargetEntry.Icon);
							Style.Normal.ImageSize = FVector2D(220.f, 80.f);
							Style.Pressed.SetResourceObject(TargetEntry.Icon);
							Style.Pressed.ImageSize = FVector2D(220.f, 80.f);
						}

						ChoiceBtn->SetStyle(Style);
					}

					// 多目标时需要点击选择
					if (Targets.Num() > 1)
					{
						ChoiceBtn->OnClicked.AddDynamic(this, &ULFPUnitMergeWidget::OnEvolutionChoiceClicked);
					}
				}

				if (Targets.Num() == 1)
				{
					// 唯一目标 → 自动选中，直接可合并
					SelectedEvolutionTarget = Targets[0];
					bCanMerge = true;

					FLFPUnitRegistryEntry TargetEntry;
					if (CachedRegistry->FindEntry(SelectedEvolutionTarget, TargetEntry) && Text_PreviewName)
					{
						FString PreviewText = FString::Printf(TEXT("%s %s"),
							*TargetEntry.DisplayName.ToString(), *MakeTierStars(TargetEntry.Tier));
						Text_PreviewName->SetText(FText::FromString(PreviewText));
					}
				}
				else
				{
					// 多目标 → 等待玩家选择
					if (Text_PreviewName)
					{
						Text_PreviewName->SetText(FText::FromString(TEXT("请选择进化方向")));
					}
				}
			}
			else
			{
				// 无进化目标（终阶）
				if (Text_PreviewName)
				{
					Text_PreviewName->SetText(FText::FromString(TEXT("已达终阶")));
				}
			}
		}
		else
		{
			if (Text_PreviewName)
			{
				Text_PreviewName->SetText(FText::FromString(TEXT("无法合并")));
			}
		}
	}
	else
	{
		if (Text_PreviewName)
		{
			Text_PreviewName->SetText(FText::FromString(TEXT("请选择两个相同单位")));
		}
	}

	if (Button_Merge)
	{
		Button_Merge->SetIsEnabled(bCanMerge);
	}
}

void ULFPUnitMergeWidget::OnEvolutionChoiceClicked()
{
	for (const auto& Pair : EvolutionButtonToTargetMap)
	{
		if (Pair.Key && Pair.Key->IsHovered())
		{
			SelectedEvolutionTarget = Pair.Value;

			// 更新所有分支按钮样式：选中的三态一致，未选中的恢复 hover/press 效果
			for (const auto& BtnPair : EvolutionButtonToTargetMap)
			{
				UButton* Btn = BtnPair.Key;
				if (!Btn) continue;

				FLFPUnitRegistryEntry Entry;
				if (!CachedRegistry || !CachedRegistry->FindEntry(BtnPair.Value, Entry) || !Entry.Icon) continue;

				FButtonStyle Style = Btn->GetStyle();
				if (Btn == Pair.Key)
				{
					// 选中：三态一致，看起来像静态图片
					Style.Normal = Style.Hovered;
					Style.Pressed = Style.Hovered;
				}
				else
				{
					// 未选中：恢复正常交互样式
					Style.Normal.SetResourceObject(Entry.Icon);
					Style.Normal.ImageSize = Style.Hovered.ImageSize;
					Style.Pressed.SetResourceObject(Entry.Icon);
					Style.Pressed.ImageSize = Style.Hovered.ImageSize;
				}
				Btn->SetStyle(Style);
				Btn->SetRenderOpacity(Btn == Pair.Key ? 1.f : 0.4f);
			}

			FLFPUnitRegistryEntry TargetEntry;
			if (CachedRegistry && CachedRegistry->FindEntry(SelectedEvolutionTarget, TargetEntry))
			{
				if (Text_PreviewName)
				{
					FString PreviewText = FString::Printf(TEXT("%s %s"),
						*TargetEntry.DisplayName.ToString(), *MakeTierStars(TargetEntry.Tier));
					Text_PreviewName->SetText(FText::FromString(PreviewText));
				}
			}

			if (Button_Merge)
			{
				Button_Merge->SetIsEnabled(true);
			}
			return;
		}
	}
}

void ULFPUnitMergeWidget::OnMergeClicked()
{
	if (!CachedGameInstance || SlotA.bIsEmpty || SlotB.bIsEmpty) return;
	if (SelectedEvolutionTarget == NAME_None) return;

	bool bSuccess = CachedGameInstance->MergeUnits(
		SlotA.bIsParty, SlotA.SlotIndex,
		SlotB.bIsParty, SlotB.SlotIndex,
		SelectedEvolutionTarget);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("升阶 Widget: 进化成功 → %s"), *SelectedEvolutionTarget.ToString());
	}

	ClearSlots();
	RefreshUnitIcons();
}

void ULFPUnitMergeWidget::OnClearClicked()
{
	ClearSlots();
	RefreshUnitIcons();
}

void ULFPUnitMergeWidget::OnCloseClicked()
{
	OnClosed.Broadcast();
	RemoveFromParent();
}

void ULFPUnitMergeWidget::UpdateSlotVisual(const FLFPMergeSlotInfo& InSlot, UImage* SlotImage)
{
	if (!SlotImage) return;

	if (InSlot.bIsEmpty)
	{
		SlotImage->SetRenderOpacity(0.2f);
		SlotImage->SetBrushFromTexture(nullptr);
	}
	else if (CachedRegistry)
	{
		FLFPUnitRegistryEntry RegEntry;
		if (CachedRegistry->FindEntry(InSlot.Unit.TypeID, RegEntry) && RegEntry.Icon)
		{
			SlotImage->SetBrushFromTexture(RegEntry.Icon);
			SlotImage->SetRenderOpacity(1.f);
		}
	}
}

FString ULFPUnitMergeWidget::MakeTierStars(int32 Tier) const
{
	FString Stars;
	// 固定显示 3 颗星位
	const int32 MaxStars = 3;
	for (int32 i = 0; i < MaxStars; ++i)
	{
		Stars += (i < Tier) ? TEXT("★") : TEXT("☆");
	}
	return Stars;
}
