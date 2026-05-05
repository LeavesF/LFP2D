// Fill out your copyright notice in the Description page of Project Settings.

#include "LFP2D/UI/Fighting/LFPBuffIconWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULFPBuffIconWidget::SetBuffEntry(const FLFPBuffDisplayEntry& Entry)
{
	BuffEntry = Entry;

	if (IconImage && Entry.Icon)
	{
		IconImage->SetBrushFromTexture(Entry.Icon);
	}

	if (StackText)
	{
		const bool bHasStackText = Entry.StackCount > 1;
		const FText StackDisplayText = Entry.StackCount > 1
			? FText::AsNumber(Entry.StackCount)
			: FText::GetEmpty();
		StackText->SetText(StackDisplayText);
		StackText->SetVisibility(bHasStackText ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (TurnsText)
	{
		const bool bHasTurnsText = Entry.RemainingTurns > 0;
		const FText TurnsDisplayText = Entry.RemainingTurns > 0
			? FText::AsNumber(Entry.RemainingTurns)
			: FText::GetEmpty();
		TurnsText->SetText(TurnsDisplayText);
		TurnsText->SetVisibility(bHasTurnsText ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetRenderOpacity(Entry.bIsActive ? 1.0f : 0.45f);

	if (!Entry.Description.IsEmpty())
	{
		SetToolTipText(FText::Format(
			NSLOCTEXT("LFPBuffIconWidget", "BuffTooltipWithDescription", "{0}\n{1}"),
			Entry.DisplayName,
			Entry.Description));
	}
	else
	{
		SetToolTipText(Entry.DisplayName);
	}

	OnBuffEntryUpdated(Entry);
}
