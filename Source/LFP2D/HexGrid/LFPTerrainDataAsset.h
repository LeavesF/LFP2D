#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPTerrainDataAsset.generated.h"

class UPaperSprite;

/**
 * 地形数据资产：定义每种地形的属性（移动代价、可行走性、显示精灵等）
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPTerrainDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 地形类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	ELFPTerrainType TerrainType = ELFPTerrainType::TT_Grass;

	// 移动代价（1 = 正常，2 = 减速地形）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MovementCost = 1;

	// 是否可行走（水域、熔岩等设为 false）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	bool bIsWalkable = true;

	// 地形基础精灵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
	TObjectPtr<UPaperSprite> DefaultSprite = nullptr;

	// 显示名称（UI 用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
	FText DisplayName;
};
