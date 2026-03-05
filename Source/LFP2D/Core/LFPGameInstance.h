#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LFPGameInstance.generated.h"

class ALFPWorldMapNode;

// 战斗请求：世界地图 → 战斗场景传递的数据
USTRUCT(BlueprintType)
struct FLFPBattleRequest
{
	GENERATED_BODY()

	// 触发战斗的节点 ID
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 SourceNodeID = -1;

	// 战斗地图 CSV 文件名（对应 Saved/Maps/ 下的 CSV）
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	FString BattleMapName;

	// 敌人星级
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 StarRating = 1;

	// 是否可逃跑
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bCanEscape = true;

	// 是否有有效请求
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bIsValid = false;
};

// 战斗结果：战斗场景 → 世界地图传递的数据
USTRUCT(BlueprintType)
struct FLFPBattleResult
{
	GENERATED_BODY()

	// 对应的节点 ID
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 SourceNodeID = -1;

	// 是否胜利
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bVictory = false;

	// 是否逃跑
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bEscaped = false;

	// 是否有有效结果
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bIsValid = false;
};

// 世界地图状态快照：跨关卡保持
USTRUCT(BlueprintType)
struct FLFPWorldMapSnapshot
{
	GENERATED_BODY()

	// 当前世界地图名
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	FString WorldMapName;

	// 玩家当前节点 ID
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	int32 CurrentNodeID = 0;

	// 已消耗回合数
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	int32 CurrentTurn = 0;

	// 已访问节点
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	TSet<int32> VisitedNodeIDs;

	// 已揭露节点
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	TSet<int32> RevealedNodeIDs;

	// 已触发节点（战斗/事件/Boss 首次触发后不再触发）
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	TSet<int32> TriggeredNodeIDs;

	// 是否有有效快照
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	bool bIsValid = false;
};

/**
 * 游戏实例：跨关卡生命周期
 * 负责世界地图 ↔ 战斗场景之间的状态传递
 */
UCLASS()
class LFP2D_API ULFPGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ============== 战斗请求/结果 ==============

	// 设置战斗请求（世界地图调用，进入战斗前）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void SetBattleRequest(const FLFPBattleRequest& Request);

	// 获取战斗请求（战斗场景调用，读取后清除）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	FLFPBattleRequest ConsumeBattleRequest();

	// 设置战斗结果（战斗场景调用，结束战斗时）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void SetBattleResult(const FLFPBattleResult& Result);

	// 获取战斗结果（世界地图调用，读取后清除）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	FLFPBattleResult ConsumeBattleResult();

	// ============== 世界地图快照 ==============

	// 保存世界地图状态快照
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void SaveWorldMapSnapshot(const FLFPWorldMapSnapshot& Snapshot);

	// 获取世界地图快照（不清除，世界地图关卡可多次读取）
	UFUNCTION(BlueprintPure, Category = "Scene Transition")
	const FLFPWorldMapSnapshot& GetWorldMapSnapshot() const { return WorldMapSnapshot; }

	// 是否有待处理的战斗结果
	UFUNCTION(BlueprintPure, Category = "Scene Transition")
	bool HasPendingBattleResult() const { return PendingBattleResult.bIsValid; }

	// ============== 场景切换 ==============

	// 切换到战斗场景
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void TransitionToBattle(const FString& BattleLevelName);

	// 切换回世界地图
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void TransitionToWorldMap(const FString& WorldMapLevelName);

	// 世界地图关卡名（蓝图中配置，默认值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Transition")
	FString DefaultWorldMapLevelName = TEXT("Test_WorldMap");

	// 战斗关卡名（蓝图中配置，默认值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Transition")
	FString DefaultBattleLevelName = TEXT("Test_Fight");

protected:
	// 待处理的战斗请求
	UPROPERTY()
	FLFPBattleRequest PendingBattleRequest;

	// 待处理的战斗结果
	UPROPERTY()
	FLFPBattleResult PendingBattleResult;

	// 世界地图状态快照
	UPROPERTY()
	FLFPWorldMapSnapshot WorldMapSnapshot;
};
