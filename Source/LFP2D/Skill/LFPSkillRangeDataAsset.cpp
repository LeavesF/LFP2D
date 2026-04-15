// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillRangeDataAsset.h"

bool ULFPSkillRangeDataAsset::FindPreset(FName PresetName, FLFPSkillRangePreset& OutPreset) const
{
    if (const FLFPSkillRangePreset* FoundPreset = Presets.Find(PresetName))
    {
        OutPreset = *FoundPreset;
        return true;
    }

    return false;
}

TArray<FName> ULFPSkillRangeDataAsset::GetPresetNames() const
{
    TArray<FName> PresetNames;
    Presets.GetKeys(PresetNames);
    PresetNames.Sort([](const FName& A, const FName& B)
    {
        return A.LexicalLess(B);
    });
    return PresetNames;
}
