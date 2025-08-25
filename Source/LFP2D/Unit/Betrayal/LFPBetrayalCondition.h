// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LFPBetrayalCondition.generated.h"

class ALFPTacticsUnit;
/**
 * 
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, meta = (DontUseGenericSpawnObject = "True", DisableNativeTick))
class LFP2D_API ULFPBetrayalCondition : public UObject
{
	GENERATED_BODY()
	
public:
	// 检查条件是否满足
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Betrayal")
	bool CheckCondition(ALFPTacticsUnit* Unit);

	// 检查条件是否满足
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Betrayal")
	bool RegisterCondition(ALFPTacticsUnit* Unit);

	// 检查条件是否满足
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Betrayal")
	bool UnRegisterCondition(ALFPTacticsUnit* Unit);

	// 条件描述（用于UI显示）
	UFUNCTION(BlueprintPure, Category = "Betrayal")
	FText GetConditionDescription() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Betrayal")
	FText ConditionDescription;
};
