#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_FetidPeck.generated.h"

class ULFPBuffDefinitionDataAsset;

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_FetidPeck : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_FetidPeck();

    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Buff", meta = (ClampMin = "1"))
    int32 PoisonStacks = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Buff", meta = (ClampMin = "1"))
    int32 PoisonDurationTurns = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Buff", meta = (ClampMin = "1"))
    int32 PoisonDamagePerMovedTile = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Buff")
    TObjectPtr<ULFPBuffDefinitionDataAsset> PoisonBuffDefinition = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ULFPBuffDefinitionDataAsset> RuntimePoisonBuffDefinition = nullptr;

private:
    ULFPBuffDefinitionDataAsset* GetPoisonBuffDefinition();
};
