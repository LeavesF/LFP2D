#include "LFP2D/Unit/Betrayal/LFPBetrayalWorldSubsystem.h"

#include "LFP2D/Unit/Betrayal/LFPBetrayalComponent.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPBetrayalWorldSubsystem::RegisterComponent(ULFPBetrayalComponent* Component)
{
	if (!Component)
	{
		return;
	}

	RegisteredComponents.AddUnique(Component);
	Component->OnBetrayalResolved.AddUniqueDynamic(this, &ULFPBetrayalWorldSubsystem::HandleComponentBetrayal);
}

void ULFPBetrayalWorldSubsystem::UnregisterComponent(ULFPBetrayalComponent* Component)
{
	if (!Component)
	{
		return;
	}

	RegisteredComponents.Remove(Component);
	Component->OnBetrayalResolved.RemoveDynamic(this, &ULFPBetrayalWorldSubsystem::HandleComponentBetrayal);
}

bool ULFPBetrayalWorldSubsystem::EvaluateUnit(ALFPTacticsUnit* Unit)
{
	if (!Unit)
	{
		return false;
	}

	RegisteredComponents.RemoveAll([](const TObjectPtr<ULFPBetrayalComponent>& Component)
	{
		return !IsValid(Component.Get());
	});

	for (const TObjectPtr<ULFPBetrayalComponent>& Component : RegisteredComponents)
	{
		if (Component && Component->GetOwnerUnit() == Unit)
		{
			return Component->EvaluateBetrayalConditions();
		}
	}

	return Unit->EvaluateBetrayalConditions();
}

int32 ULFPBetrayalWorldSubsystem::EvaluateAllUnits()
{
	RegisteredComponents.RemoveAll([](const TObjectPtr<ULFPBetrayalComponent>& Component)
	{
		return !IsValid(Component.Get());
	});

	const TArray<TObjectPtr<ULFPBetrayalComponent>> ComponentsSnapshot = RegisteredComponents;
	int32 BetrayalCount = 0;
	for (const TObjectPtr<ULFPBetrayalComponent>& Component : ComponentsSnapshot)
	{
		if (Component && Component->EvaluateBetrayalConditions())
		{
			BetrayalCount++;
		}
	}

	return BetrayalCount;
}

void ULFPBetrayalWorldSubsystem::HandleComponentBetrayal(ALFPTacticsUnit* Unit, EUnitAffiliation OldAffiliation, ULFPBetrayalCondition* TriggeringCondition)
{
    OnBetrayalResolvedInSubsystem.Broadcast(Unit, OldAffiliation, TriggeringCondition);
}
