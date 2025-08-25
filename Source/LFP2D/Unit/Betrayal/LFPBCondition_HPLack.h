// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Unit/Betrayal/LFPBetrayalCondition.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPBCondition_HPLack.generated.h"

/**
 * 
 */
UCLASS()
class LFP2D_API ULFPBCondition_HPLack : public ULFPBetrayalCondition
{
	GENERATED_BODY()

public:
	virtual bool CheckCondition_Implementation(ALFPTacticsUnit* Unit) override
	{
		return Unit && Unit->GetCurrentHealth() <= HealthThreshold;
	}

private:
	UPROPERTY(EditAnywhere, Category = "Condition")
	int32 HealthThreshold = 3;
};
