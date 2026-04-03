#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/Shop/LFPHireMarketTypes.h"
#include "LFPHireMarketDataAsset.generated.h"

/**
 * 雇佣市场配置表：HireMarketID -> 雇佣市场定义
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPHireMarketDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HireMarket")
	TMap<FName, FLFPHireMarketDefinition> HireMarketMap;

	UFUNCTION(BlueprintPure, Category = "HireMarket")
	bool FindHireMarketDefinition(FName HireMarketID, FLFPHireMarketDefinition& OutDefinition) const;
};
