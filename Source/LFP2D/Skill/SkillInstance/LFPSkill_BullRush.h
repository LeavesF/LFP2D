#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_BullRush.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_BullRush : public ULFPSkillBase
{
	GENERATED_BODY()

public:
	ULFPSkill_BullRush();

	virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual float GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const override;
	virtual ELFPAttackType GetDamageType_Implementation(ALFPTacticsUnit* Target) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float DamageScale = 1.0f;

private:
	bool GetRushDirection(ALFPHexTile* TargetTile, FLFPHexCoordinates& OutDirection) const;
	ALFPHexTile* GetBullRushDestination(ALFPHexTile* TargetTile, const FLFPHexCoordinates& Direction, int32 PushDistance) const;
};
