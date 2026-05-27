#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_SelfApplyBuff.generated.h"

class ULFPBuffDefinitionDataAsset;

// 给自己施加 Buff 的通用技能基类。
// 蓝图继承后只需配置 BuffDefinition 即可使用，无需编写 C++ 子类。
UCLASS(Abstract, Blueprintable)
class LFP2D_API ULFPSkill_SelfApplyBuff : public ULFPSkillBase
{
	GENERATED_BODY()

public:
	ULFPSkill_SelfApplyBuff();

	virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
	virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Buff")
	TObjectPtr<ULFPBuffDefinitionDataAsset> BuffDefinition = nullptr;
};
