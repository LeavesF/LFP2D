#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_LoneWolf.generated.h"

class ULFPBuffDefinitionDataAsset;

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_LoneWolf : public ULFPSkillBase
{
	GENERATED_BODY()

public:
	ULFPSkill_LoneWolf();

	virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Passive")
	TObjectPtr<ULFPBuffDefinitionDataAsset> PassiveBuffDefinition = nullptr;
};
