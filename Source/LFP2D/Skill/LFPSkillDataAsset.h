// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFPSkillDataAsset.generated.h"

class ULFPSkillBase;
class ALFPTacticsUnit;
/**
 * 
 */
UCLASS()
class LFP2D_API ULFPSkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    // ��λ���õļ�����
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
    TArray<TSubclassOf<ULFPSkillBase>> AvailableSkills;

    // ���ݵ�λ���ͻ�ȡ��������
    UFUNCTION(BlueprintCallable, Category = "Skills")
    static ULFPSkillDataAsset* GetSkillDataForUnitType(TSubclassOf<ALFPTacticsUnit> UnitType);
};
