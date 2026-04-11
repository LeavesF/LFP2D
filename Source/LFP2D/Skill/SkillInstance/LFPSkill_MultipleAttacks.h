#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_MultipleAttacks.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_MultipleAttacks : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_MultipleAttacks();

    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
};
