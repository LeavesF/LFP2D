#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/Shop/LFPRelicTypes.h"
#include "LFPRelicDataAsset.generated.h"

/**
 * 遗物注册表：RelicID -> 遗物定义
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPRelicDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	TMap<FName, FLFPRelicDefinition> RelicMap;

	// 遗物组合规则
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	TArray<FLFPRelicSynergyRule> SynergyRules;

	UFUNCTION(BlueprintPure, Category = "Relic")
	bool FindRelicDefinition(FName RelicID, FLFPRelicDefinition& OutDefinition) const;
};
