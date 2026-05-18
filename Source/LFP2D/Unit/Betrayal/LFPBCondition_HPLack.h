// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Unit/Betrayal/LFPBetrayalCondition.h"
#include "LFPBCondition_HPLack.generated.h"

class ALFPTacticsUnit;

/**
 * 
 */
UCLASS()
class LFP2D_API ULFPBCondition_HPLack : public ULFPBetrayalCondition
{
	GENERATED_BODY()

public:
	virtual bool CheckCondition_Implementation(ALFPTacticsUnit* Unit) override;
	virtual bool RegisterCondition_Implementation(ALFPTacticsUnit* Unit) override;
	virtual bool UnRegisterCondition_Implementation(ALFPTacticsUnit* Unit) override;

private:
	UFUNCTION()
	void HandleRegisteredUnitHealthChanged(ALFPTacticsUnit* Unit, int32 CurrentHealth, int32 MaxHealth);

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0"))
	int32 HealthThreshold = 3;

	UPROPERTY(Transient)
	TObjectPtr<ALFPTacticsUnit> RegisteredUnit;
};
