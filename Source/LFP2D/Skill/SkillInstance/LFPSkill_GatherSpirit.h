#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_GatherSpirit.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_GatherSpirit : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_GatherSpirit();

    virtual bool CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile) override;
    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "1"))
    int32 RestoreAmount = 1;
};
