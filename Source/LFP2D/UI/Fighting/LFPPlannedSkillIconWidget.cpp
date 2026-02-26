// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/UI/Fighting/LFPPlannedSkillIconWidget.h"
#include "Components/Image.h"

void ULFPPlannedSkillIconWidget::SetSkillIcon(UTexture2D* IconTexture)
{
    if (SkillIconImage && IconTexture)
    {
        SkillIconImage->SetBrushFromTexture(IconTexture);
        SkillIconImage->SetVisibility(ESlateVisibility::Visible);
    }
    else if (SkillIconImage)
    {
        SkillIconImage->SetVisibility(ESlateVisibility::Hidden);
    }
}
