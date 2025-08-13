// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/UI/Fighting/LFPTurnSpeedIconWidget.h"

void ULFPTurnSpeedIconWidget::SetUnit(ALFPTacticsUnit* Unit)
{
    UnitRef = Unit;
    UpdateAppearance();
}

void ULFPTurnSpeedIconWidget::SetIsCurrent(bool bCurrent)
{
    bIsCurrent = bCurrent;
    UpdateAppearance();
}