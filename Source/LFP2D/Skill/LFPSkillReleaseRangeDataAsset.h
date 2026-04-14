// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFPSkillReleaseRangeDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FLFPSkillReleaseRangePreset
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Range")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Range")
    TArray<FLFPHexCoordinates> ReleaseRangeCoords;
};

UCLASS(BlueprintType)
class LFP2D_API ULFPSkillReleaseRangeDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Range")
    TMap<FName, FLFPSkillReleaseRangePreset> Presets;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill Range")
    bool FindPreset(FName PresetName, FLFPSkillReleaseRangePreset& OutPreset) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill Range")
    TArray<FName> GetPresetNames() const;
};
