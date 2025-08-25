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
	// ��������Ƿ�����
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Betrayal")
	bool CheckCondition(ALFPTacticsUnit* Unit);

	// ��������Ƿ�����
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Betrayal")
	bool RegisterCondition(ALFPTacticsUnit* Unit);

	// ��������Ƿ�����
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Betrayal")
	bool UnRegisterCondition(ALFPTacticsUnit* Unit);

	// ��������������UI��ʾ��
	UFUNCTION(BlueprintPure, Category = "Betrayal")
	FText GetConditionDescription() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Betrayal")
	FText ConditionDescription;
};
