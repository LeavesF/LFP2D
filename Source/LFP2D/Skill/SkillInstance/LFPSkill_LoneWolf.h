#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_LoneWolf.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_LoneWolf : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_LoneWolf();

    virtual void RegisterPassiveBuffs_Implementation(ALFPTacticsUnit* InOwner) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Passive", meta = (ClampMin = "0"))
    int32 FriendlyCheckRange = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Passive")
    int32 AttackBonus = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Passive")
    int32 PhysicalBlockBonus = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Passive")
    int32 SpeedBonus = 3;
};
