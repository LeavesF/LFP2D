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
    // 单位可用的技能类
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    TArray<TSubclassOf<ULFPSkillBase>> AvailableSkills;

    // 根据单位类型获取技能数据
    UFUNCTION(BlueprintCallable, Category = "Skills")
    static ULFPSkillDataAsset* GetSkillDataForUnitType(TSubclassOf<ALFPTacticsUnit> UnitType);
};
