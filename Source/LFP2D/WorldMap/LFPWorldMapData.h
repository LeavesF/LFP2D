#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LFPWorldMapData.generated.h"

// 世界地图节点类型
UENUM(BlueprintType)
enum class ELFPWorldNodeType : uint8
{
	WNT_Battle      UMETA(DisplayName = "战斗"),
	WNT_Event       UMETA(DisplayName = "事件"),
	WNT_Shop        UMETA(DisplayName = "商店"),
	WNT_Town        UMETA(DisplayName = "城镇"),
	WNT_Boss        UMETA(DisplayName = "Boss"),
	WNT_QuestNPC    UMETA(DisplayName = "任务NPC"),
	WNT_SkillNode   UMETA(DisplayName = "技能节点"),
	WNT_EvolutionTower UMETA(DisplayName = "升华塔")
};

/**
 * 世界地图节点行数据（DataTable / CSV 行结构）
 * RowName 使用 NodeID 字符串，如 "0", "1", "42"
 */
USTRUCT(BlueprintType)
struct FLFPWorldNodeRow : public FTableRowBase
{
	GENERATED_BODY()

	// 节点唯一 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node")
	int32 NodeID = 0;

	// 世界坐标 X（自由 2D 坐标，非六角）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node")
	float PosX = 0.f;

	// 世界坐标 Y
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node")
	float PosY = 0.f;

	// 节点类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node")
	ELFPWorldNodeType NodeType = ELFPWorldNodeType::WNT_Battle;

	// ==== 战斗节点参数 ====

	// 关联战斗地图文件名（对应 Saved/Maps/ 下的 CSV）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	FString BattleMapName;

	// 敌人星级（1~5）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle", meta = (ClampMin = "1", ClampMax = "5"))
	int32 StarRating = 1;

	// 是否可以逃跑
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	bool bCanEscape = true;

	// 基础金币奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	int32 BaseGoldReward = 0;

	// 基础食物奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	int32 BaseFoodReward = 0;

	// ==== 事件节点参数 ====

	// 事件 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Event")
	FString EventID;

	// ==== 城镇节点参数 ====

	// 城镇建筑列表（分号分隔，如 "Shop;EvolutionTower;Teleport"）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Town")
	FString TownBuildingList;

	// ==== 解锁条件 ====

	// 前置节点 ID 列表（分号分隔，如 "0;3;5"）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node")
	FString PrerequisiteNodeIDs;
};

/**
 * 世界地图边行数据（DataTable / CSV 行结构）
 * RowName 使用 "FromID_ToID" 格式，如 "0_1"
 */
USTRUCT(BlueprintType)
struct FLFPWorldEdgeRow : public FTableRowBase
{
	GENERATED_BODY()

	// 起点节点 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Edge")
	int32 FromNodeID = 0;

	// 终点节点 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Edge")
	int32 ToNodeID = 0;

	// 移动回合消耗（整数）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Edge", meta = (ClampMin = "1"))
	int32 TravelTurnCost = 1;
};
