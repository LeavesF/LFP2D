#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_PlagueSpread.generated.h"

class ALFPTacticsUnit;
class ULFPBuffComponent;

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_PlagueSpread : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_PlagueSpread();

    virtual bool CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile) override;
    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0"))
    int32 SpreadRange = 3;

private:
    bool HasSpreadableDebuffs(ALFPTacticsUnit* SourceUnit) const;
    bool HasSpreadTargetInRange(ALFPTacticsUnit* SourceUnit) const;
    int32 SpreadDebuffsFromTarget(ALFPTacticsUnit* SourceUnit) const;
    int32 CopyDebuffsToTarget(const ULFPBuffComponent* SourceBuffComponent, ALFPTacticsUnit* RecipientUnit) const;
};
