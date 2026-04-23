#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_LockRemoteAttack.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_LockRemoteAttack : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_LockRemoteAttack();

    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual float GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const override;
    virtual ELFPAttackType GetDamageType_Implementation(ALFPTacticsUnit* Target) const override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float DamageScale = 1.0f;
};
