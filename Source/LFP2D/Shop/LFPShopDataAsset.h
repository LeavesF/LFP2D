#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/Shop/LFPRelicTypes.h"
#include "LFPShopDataAsset.generated.h"

/**
 * 商店配置表：ShopID -> 商店定义
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPShopDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TMap<FName, FLFPShopDefinition> ShopMap;

	UFUNCTION(BlueprintPure, Category = "Shop")
	bool FindShopDefinition(FName ShopID, FLFPShopDefinition& OutDefinition) const;
};
