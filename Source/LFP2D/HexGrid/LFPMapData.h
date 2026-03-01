#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPMapData.generated.h"

// 出生点阵营枚举（独立于 EUnitAffiliation，加入"无"标记）
UENUM(BlueprintType)
enum class ELFPSpawnFaction : uint8
{
	SF_None     UMETA(DisplayName = "无"),
	SF_Player   UMETA(DisplayName = "玩家"),
	SF_Enemy    UMETA(DisplayName = "敌方"),
	SF_Neutral  UMETA(DisplayName = "中立")
};

/**
 * 地图格子行数据（DataTable / CSV 行结构）
 * RowName 使用 "Q_R" 格式，如 "3_-2"
 */
USTRUCT(BlueprintType)
struct FLFPMapTileRow : public FTableRowBase
{
	GENERATED_BODY()

	// 立方坐标 Q
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Tile")
	int32 Q = 0;

	// 立方坐标 R
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Tile")
	int32 R = 0;

	// 地形类型（加载时映射到地形数据资产）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Tile")
	ELFPTerrainType TerrainType = ELFPTerrainType::TT_Grass;

	// 装饰标识（空 = 无装饰，对应装饰注册表中的 Key）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Tile")
	FName DecorationID = NAME_None;

	// 出生点阵营
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Tile")
	ELFPSpawnFaction SpawnFaction = ELFPSpawnFaction::SF_None;

	// 出生点顺序索引（同一阵营内的出生顺序，0 = 非出生点）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Tile")
	int32 SpawnIndex = 0;

	// 事件标签（空 = 无事件）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Tile")
	FGameplayTag EventTag;
};
