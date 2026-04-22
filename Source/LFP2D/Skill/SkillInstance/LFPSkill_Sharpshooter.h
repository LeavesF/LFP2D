#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_Sharpshooter.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_Sharpshooter : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_Sharpshooter();

    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual float GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const override;
    virtual ELFPAttackType GetDamageType_Implementation(ALFPTacticsUnit* Target) const override;
    // 按目标实际距离返回暴击率：2 格 50%，每远 1 格下降 10%。
    virtual float GetCriticalChance_Implementation(ALFPTacticsUnit* Target) const override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float DamageScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CloseRangeCriticalChance = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CriticalChanceFalloffPerHex = 0.1f;

private:
    // 将距离映射为神射的暴击率。
    float GetCriticalChanceAtDistance(int32 Distance) const;
};
