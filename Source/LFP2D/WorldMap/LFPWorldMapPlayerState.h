#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LFPWorldMapPlayerState.generated.h"

class ALFPWorldMapManager;
class ALFPWorldMapNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldMapTurnChanged, int32, CurrentTurn, int32, TurnBudget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerNodeChanged, ALFPWorldMapNode*, NewNode);

/**
 * 世界地图玩家状态：追踪位置、回合消耗、访问/揭露节点
 * 由 WorldMapManager 持有
 */
UCLASS(BlueprintType)
class LFP2D_API ULFPWorldMapPlayerState : public UObject
{
	GENERATED_BODY()

public:
	// 当前所在节点 ID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Map")
	int32 CurrentNodeID = 0;

	// 已消耗的总回合数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Map")
	int32 CurrentTurn = 0;

	// 回合压力阈值（超过后全局难度递增）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Map")
	int32 TurnPressureThreshold = 100;

	// 已访问节点集合
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Map")
	TSet<int32> VisitedNodeIDs;

	// 已揭露节点集合（永久可见）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Map")
	TSet<int32> RevealedNodeIDs;

	// ============== 方法 ==============

	// 移动到相邻节点（验证边存在、消耗回合、更新位置）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	bool MoveToNode(int32 TargetNodeID, ALFPWorldMapManager* Manager);

	// 获取从当前节点出发的可达节点 ID 列表
	UFUNCTION(BlueprintCallable, Category = "World Map")
	TArray<int32> GetReachableNodeIDs(ALFPWorldMapManager* Manager) const;

	// 是否超过压力阈值
	UFUNCTION(BlueprintPure, Category = "World Map")
	bool IsPastPressureThreshold() const { return CurrentTurn >= TurnPressureThreshold; }

	// 初始化（设置起始节点）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void Initialize(int32 StartNodeID);

	// ============== 委托 ==============

	UPROPERTY(BlueprintAssignable)
	FOnWorldMapTurnChanged OnTurnChanged;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerNodeChanged OnPlayerNodeChanged;
};
