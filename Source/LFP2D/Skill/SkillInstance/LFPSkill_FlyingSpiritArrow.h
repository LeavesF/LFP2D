#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_FlyingSpiritArrow.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_FlyingSpiritArrow : public ULFPSkillBase
{
	GENERATED_BODY()

public:
	ULFPSkill_FlyingSpiritArrow();

	virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual float GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const override;
	virtual ELFPAttackType GetDamageType_Implementation(ALFPTacticsUnit* Target) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float DamageScale = 1.0f;

private:
	bool GetRayDirectionToTarget(ALFPHexTile* TargetTile, FLFPHexCoordinates& OutDirection) const;
	ALFPTacticsUnit* FindFirstAliveUnitOnRay(const FLFPHexCoordinates& Direction, ALFPHexTile*& OutHitTile) const;
	void MoveOwnerToTile(ALFPHexTile* DestinationTile) const;
};
