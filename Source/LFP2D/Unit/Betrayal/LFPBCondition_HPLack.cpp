// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Unit/Betrayal/LFPBCondition_HPLack.h"

#include "LFP2D/Unit/LFPTacticsUnit.h"

bool ULFPBCondition_HPLack::CheckCondition_Implementation(ALFPTacticsUnit* Unit)
{
	return Unit && Unit->GetCurrentHealth() <= HealthThreshold;
}

bool ULFPBCondition_HPLack::RegisterCondition_Implementation(ALFPTacticsUnit* Unit)
{
	if (!Unit)
	{
		return false;
	}

	if (RegisteredUnit && RegisteredUnit != Unit)
	{
		UnRegisterCondition(RegisteredUnit);
	}

	RegisteredUnit = Unit;
	RegisteredUnit->OnHealthChangedWithUnitDelegate.AddUniqueDynamic(this, &ULFPBCondition_HPLack::HandleRegisteredUnitHealthChanged);
	return true;
}

bool ULFPBCondition_HPLack::UnRegisterCondition_Implementation(ALFPTacticsUnit* Unit)
{
	ALFPTacticsUnit* UnitToUnregister = Unit ? Unit : RegisteredUnit.Get();
	if (!UnitToUnregister)
	{
		return false;
	}

	UnitToUnregister->OnHealthChangedWithUnitDelegate.RemoveDynamic(this, &ULFPBCondition_HPLack::HandleRegisteredUnitHealthChanged);
	if (RegisteredUnit == UnitToUnregister)
	{
		RegisteredUnit = nullptr;
	}

	return true;
}

void ULFPBCondition_HPLack::HandleRegisteredUnitHealthChanged(ALFPTacticsUnit* Unit, int32 CurrentHealth, int32 MaxHealth)
{
	if (!Unit || Unit != RegisteredUnit)
	{
		return;
	}

	Unit->EvaluateBetrayalConditions();
}
