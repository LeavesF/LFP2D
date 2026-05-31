#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_WoodRootEntangle.generated.h"

class ULFPBuffDefinitionDataAsset;

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_WoodRootEntangle : public ULFPSkillBase
{
	GENERATED_BODY()

public:
	ULFPSkill_WoodRootEntangle();

	virtual bool CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile) override;
	virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual void UpdateSkillRange_Implementation() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (ClampMin = "0"))
	int32 EntangleRange = 2;

	// Timed buffs tick down at the target's turn start. Two turns keeps this debuff active through the target's next movement phase.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Buff", meta = (ClampMin = "2"))
	int32 RootedDurationTurns = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Buff")
	TObjectPtr<ULFPBuffDefinitionDataAsset> RootedBuffDefinition = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULFPBuffDefinitionDataAsset> RuntimeRootedBuffDefinition = nullptr;

private:
	ULFPBuffDefinitionDataAsset* GetRootedBuffDefinition();
	bool HasHostileUnitInRange(ALFPHexTile* OriginTile) const;
	int32 ApplyRootedToHostileUnitsInRange(ALFPHexTile* OriginTile);
};
