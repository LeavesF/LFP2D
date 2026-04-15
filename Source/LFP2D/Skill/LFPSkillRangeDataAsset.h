// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFPSkillRangeDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FLFPSkillRangePreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Range")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Range")
	TArray<FLFPHexCoordinates> RangeCoords;
};

UCLASS(BlueprintType)
class LFP2D_API ULFPSkillRangeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Range")
	TMap<FName, FLFPSkillRangePreset> Presets;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill Range")
	bool FindPreset(FName PresetName, FLFPSkillRangePreset& OutPreset) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill Range")
	TArray<FName> GetPresetNames() const;
};
