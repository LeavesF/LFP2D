// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillReleaseRangeDataAsset.h"

bool ULFPSkillReleaseRangeDataAsset::FindPreset(FName PresetName, FLFPSkillReleaseRangePreset& OutPreset) const
{
    if (const FLFPSkillReleaseRangePreset* FoundPreset = Presets.Find(PresetName))
    {
        OutPreset = *FoundPreset;
        return true;
    }

    return false;
}

TArray<FName> ULFPSkillReleaseRangeDataAsset::GetPresetNames() const
{
    TArray<FName> PresetNames;
    Presets.GetKeys(PresetNames);
    PresetNames.Sort([](const FName& A, const FName& B)
    {
        return A.LexicalLess(B);
    });
    return PresetNames;
}
