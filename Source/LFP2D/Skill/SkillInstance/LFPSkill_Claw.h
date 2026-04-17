#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_Claw.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_Claw : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_Claw();

    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float DamageScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 BleedStacks = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 BleedDurationTurns = 2;
};
