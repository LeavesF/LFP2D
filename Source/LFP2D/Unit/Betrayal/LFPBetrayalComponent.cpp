#include "LFP2D/Unit/Betrayal/LFPBetrayalComponent.h"

#include "LFP2D/Unit/Betrayal/LFPBetrayalCondition.h"
#include "LFP2D/Unit/Betrayal/LFPBetrayalWorldSubsystem.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPBetrayalComponent::ULFPBetrayalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULFPBetrayalComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (ULFPBetrayalWorldSubsystem* BetrayalSubsystem = World->GetSubsystem<ULFPBetrayalWorldSubsystem>())
		{
			BetrayalSubsystem->RegisterComponent(this);
		}
	}

	RegisterBetrayalConditions();
}

void ULFPBetrayalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULFPBetrayalWorldSubsystem* BetrayalSubsystem = World->GetSubsystem<ULFPBetrayalWorldSubsystem>())
		{
			BetrayalSubsystem->UnregisterComponent(this);
		}
	}

	UnregisterBetrayalConditions();

	Super::EndPlay(EndPlayReason);
}

void ULFPBetrayalComponent::ConfigureFromTemplates(const TArray<TObjectPtr<ULFPBetrayalCondition>>& ConditionTemplates)
{
	const bool bRegisterAfterRebuild = GetOwnerUnit() && GetOwnerUnit()->HasActorBegunPlay();

	if (bBetrayalConditionsRegistered)
	{
		UnregisterBetrayalConditions();
	}

	BetrayalConditions.Reset();
	BetrayalConditions.Reserve(ConditionTemplates.Num());

	for (const TObjectPtr<ULFPBetrayalCondition>& TemplateCondition : ConditionTemplates)
	{
		if (!TemplateCondition)
		{
			continue;
		}

		ULFPBetrayalCondition* RuntimeCondition = DuplicateObject<ULFPBetrayalCondition>(TemplateCondition.Get(), this);
		if (RuntimeCondition)
		{
			BetrayalConditions.Add(RuntimeCondition);
		}
	}

	if (bRegisterAfterRebuild)
	{
		RegisterBetrayalConditions();
	}
}

bool ULFPBetrayalComponent::EvaluateBetrayalConditions()
{
	ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
	if (!OwnerUnit || !OwnerUnit->IsAlive() || OwnerUnit->GetAffiliation() == EUnitAffiliation::UA_Player)
	{
		return false;
	}

	for (TObjectPtr<ULFPBetrayalCondition>& Condition : BetrayalConditions)
	{
		if (Condition && Condition->CheckCondition(OwnerUnit))
		{
			return TryBetrayToPlayer(Condition.Get());
		}
	}

	return false;
}

bool ULFPBetrayalComponent::TryBetrayToPlayer(ULFPBetrayalCondition* TriggeringCondition)
{
	ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
	if (!OwnerUnit || !OwnerUnit->IsAlive() || OwnerUnit->GetAffiliation() == EUnitAffiliation::UA_Player)
	{
		return false;
	}

	const EUnitAffiliation OldAffiliation = OwnerUnit->GetAffiliation();
	OwnerUnit->ChangeAffiliation(EUnitAffiliation::UA_Player);
	UnregisterBetrayalConditions();

	OnBetrayalResolved.Broadcast(OwnerUnit, OldAffiliation, TriggeringCondition);
	OwnerUnit->OnUnitBetrayedDelegate.Broadcast(OwnerUnit, OldAffiliation, TriggeringCondition);
	return true;
}

ALFPTacticsUnit* ULFPBetrayalComponent::GetOwnerUnit() const
{
	return Cast<ALFPTacticsUnit>(GetOwner());
}

void ULFPBetrayalComponent::RegisterBetrayalConditions()
{
	if (bBetrayalConditionsRegistered)
	{
		return;
	}

	ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
	if (!OwnerUnit)
	{
		return;
	}

	for (TObjectPtr<ULFPBetrayalCondition>& Condition : BetrayalConditions)
	{
		if (Condition)
		{
			Condition->RegisterCondition(OwnerUnit);
		}
	}

	bBetrayalConditionsRegistered = true;
	EvaluateBetrayalConditions();
}

void ULFPBetrayalComponent::UnregisterBetrayalConditions()
{
	if (!bBetrayalConditionsRegistered)
	{
		return;
	}

	ALFPTacticsUnit* OwnerUnit = GetOwnerUnit();
	for (TObjectPtr<ULFPBetrayalCondition>& Condition : BetrayalConditions)
	{
		if (Condition)
		{
			Condition->UnRegisterCondition(OwnerUnit);
		}
	}

	bBetrayalConditionsRegistered = false;
}
