#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_SwapPosition.generated.h"

class ALFPTacticsUnit;

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_SwapPosition : public ULFPSkillBase
{
	GENERATED_BODY()

public:
	ULFPSkill_SwapPosition();

	virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;

private:
	bool SwapWithTarget(ALFPTacticsUnit* TargetUnit) const;
};
