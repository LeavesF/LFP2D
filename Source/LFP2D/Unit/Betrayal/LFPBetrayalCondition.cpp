// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Unit/Betrayal/LFPBetrayalCondition.h"

bool ULFPBetrayalCondition::CheckCondition_Implementation(ALFPTacticsUnit* Unit)
{
	return false;
}

bool ULFPBetrayalCondition::RegisterCondition_Implementation(ALFPTacticsUnit* Unit)
{
	return false;
}

bool ULFPBetrayalCondition::UnRegisterCondition_Implementation(ALFPTacticsUnit* Unit)
{
	return false;
}

FText ULFPBetrayalCondition::GetConditionDescription() const
{
	return ConditionDescription;
}
