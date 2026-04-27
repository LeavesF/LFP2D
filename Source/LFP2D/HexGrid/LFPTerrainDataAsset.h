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

	// 是否阻挡弩箭类投射物（巨石、树林设为 true，草地/水域等设 false）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	bool bBlockProjectile = false;

	// 地形基础精灵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
	TObjectPtr<UPaperSprite> DefaultSprite = nullptr;

	// 显示名称（UI 用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
	FText DisplayName;

	// 渲染优先级（数值越大越优先，高优先级地形覆盖低优先级边缘）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
	int32 RenderPriority = 0;

	// 地形 Tileable 纹理（用于 World-Space UV 连续渲染）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain|Visuals")
	TObjectPtr<UTexture2D> TerrainTexture = nullptr;
};
