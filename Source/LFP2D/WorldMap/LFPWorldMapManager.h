#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/WorldMap/LFPWorldMapData.h"
#include "LFPWorldMapManager.generated.h"

class ALFPWorldMapNode;
class ALFPWorldMapEdge;
class ALFPWorldMapPawn;
class ULFPWorldNodeDataAsset;
class ULFPWorldMapPlayerState;
class UPaperSprite;

UCLASS()
class LFP2D_API ALFPWorldMapManager : public AActor
{
	GENERATED_BODY()

public:
	ALFPWorldMapManager();

protected:
	virtual void BeginPlay() override;

public:
	// ============== 节点管理 ==============

	// 生成节点
	UFUNCTION(BlueprintCallable, Category = "World Map")
	ALFPWorldMapNode* SpawnNode(int32 NodeID, FVector2D Position, ELFPWorldNodeType NodeType);

	// 移除节点（同时移除关联边）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	bool RemoveNode(int32 NodeID);

	// 按 ID 查找节点
	UFUNCTION(BlueprintPure, Category = "World Map")
	ALFPWorldMapNode* GetNode(int32 NodeID) const;

	// 获取节点的所有邻居
	UFUNCTION(BlueprintCallable, Category = "World Map")
	TArray<ALFPWorldMapNode*> GetNeighbors(int32 NodeID) const;

	// 生成下一个可用 NodeID
	UFUNCTION(BlueprintPure, Category = "World Map")
	int32 GetNextNodeID() const;

	// ============== 边管理 ==============

	// 添加边
	UFUNCTION(BlueprintCallable, Category = "World Map")
	ALFPWorldMapEdge* AddEdge(int32 FromID, int32 ToID, int32 TurnCost = 1);

	// 移除边
	UFUNCTION(BlueprintCallable, Category = "World Map")
	bool RemoveEdge(int32 FromID, int32 ToID);

	// 获取两节点之间的边
	UFUNCTION(BlueprintPure, Category = "World Map")
	ALFPWorldMapEdge* GetEdge(int32 FromID, int32 ToID) const;

	// ============== 地图操作 ==============

	// 清除所有节点和边
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void ClearMap();

	// ============== CSV 保存/加载 ==============

	UFUNCTION(BlueprintCallable, Category = "World Map")
	bool SaveWorldMap(const FString& MapName);

	UFUNCTION(BlueprintCallable, Category = "World Map")
	bool LoadWorldMap(const FString& MapName);

	UFUNCTION(BlueprintCallable, Category = "World Map")
	TArray<FString> GetSavedWorldMapList();

	// ============== 导出数据 ==============

	UFUNCTION(BlueprintCallable, Category = "World Map")
	TArray<FLFPWorldNodeRow> ExportNodeData() const;

	UFUNCTION(BlueprintCallable, Category = "World Map")
	TArray<FLFPWorldEdgeRow> ExportEdgeData() const;

	// ============== 只读引用 ==============

	const TMap<int32, ALFPWorldMapNode*>& GetNodeMap() const { return NodeMap; }
	const TMap<FIntPoint, ALFPWorldMapEdge*>& GetEdgeMap() const { return EdgeMap; }

	// ============== 视觉注册表 ==============

	// 获取节点类型对应的视觉数据
	UFUNCTION(BlueprintPure, Category = "World Map")
	ULFPWorldNodeDataAsset* GetNodeVisualForType(ELFPWorldNodeType NodeType) const;

	// ============== 迷雾系统 ==============

	// 更新迷雾状态（根据玩家位置 BFS 揭开节点）
	UFUNCTION(BlueprintCallable, Category = "World Map|Fog")
	void UpdateFog();

	// 获取从指定节点出发、图距离不超过 MaxDistance 的所有节点
	UFUNCTION(BlueprintCallable, Category = "World Map|Fog")
	TSet<int32> GetNodesWithinGraphDistance(int32 StartNodeID, int32 MaxDistance) const;

	// ============== 玩家状态 ==============

	// 获取玩家状态
	UFUNCTION(BlueprintPure, Category = "World Map")
	ULFPWorldMapPlayerState* GetPlayerState() const { return PlayerState; }

	// 初始化玩家（设置起始节点并更新迷雾）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void InitializePlayer(int32 StartNodeID);

	// 移动玩家到相邻节点
	UFUNCTION(BlueprintCallable, Category = "World Map")
	bool MovePlayer(int32 TargetNodeID);

	// 获取当前加载的世界地图名
	UFUNCTION(BlueprintPure, Category = "World Map")
	FString GetCurrentWorldMapName() const { return CurrentWorldMapName; }

	// 设置当前世界地图名
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void SetCurrentWorldMapName(const FString& MapName) { CurrentWorldMapName = MapName; }

	// 从快照恢复运行时状态（已触发节点）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void RestoreTriggeredNodes(const TSet<int32>& TriggeredNodeIDs);

	// 棋子是否正在移动中
	UFUNCTION(BlueprintPure, Category = "World Map")
	bool IsPawnMoving() const;

	// 获取玩家棋子
	UFUNCTION(BlueprintPure, Category = "World Map")
	ALFPWorldMapPawn* GetPlayerPawn() const { return PlayerPawn; }

protected:
	// 节点注册表：NodeID → Node Actor
	UPROPERTY()
	TMap<int32, ALFPWorldMapNode*> NodeMap;

	// 边注册表：EdgeKey → Edge Actor（EdgeKey = MakeEdgeKey(From, To)）
	UPROPERTY()
	TMap<FIntPoint, ALFPWorldMapEdge*> EdgeMap;

	// 邻接表：NodeID → 相邻 NodeID 集合
	TMap<int32, TSet<int32>> AdjacencyList;

	// 节点 → 关联的 EdgeKey 列表（用于删除节点时清理边）
	TMap<int32, TArray<FIntPoint>> NodeEdgeIndex;

	// Node Actor 子类（蓝图中配置）
	UPROPERTY(EditDefaultsOnly, Category = "World Map")
	TSubclassOf<ALFPWorldMapNode> NodeActorClass;

	// Edge Actor 子类（蓝图中配置）
	UPROPERTY(EditDefaultsOnly, Category = "World Map")
	TSubclassOf<ALFPWorldMapEdge> EdgeActorClass;

	// 节点类型视觉注册表（蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Map|Registry")
	TMap<ELFPWorldNodeType, TObjectPtr<ULFPWorldNodeDataAsset>> NodeVisualRegistry;

	// 边精灵（所有边共用，蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Map|Registry")
	TObjectPtr<UPaperSprite> EdgeSprite;

	// 迷雾视野范围（图距离，经过几条边）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Map|Fog")
	int32 FogVisionRange = 2;

	// 玩家状态
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Map")
	TObjectPtr<ULFPWorldMapPlayerState> PlayerState;

	// 玩家棋子类（蓝图中配置）
	UPROPERTY(EditDefaultsOnly, Category = "World Map|Pawn")
	TSubclassOf<ALFPWorldMapPawn> PawnActorClass;

	// 玩家棋子实例
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Map|Pawn")
	TObjectPtr<ALFPWorldMapPawn> PlayerPawn;

	// ID 计数器
	int32 NextNodeID = 0;

	// 当前世界地图名
	FString CurrentWorldMapName;

	// 世界地图保存目录
	FString GetWorldMapSaveDirectory() const;

	// 生成无向边的统一 Key（始终 Min, Max）
	static FIntPoint MakeEdgeKey(int32 A, int32 B);

private:
	// CSV 读写辅助
	bool SaveNodesToCSV(const FString& FilePath);
	bool SaveEdgesToCSV(const FString& FilePath);
	bool LoadNodesFromCSV(const FString& FilePath);
	bool LoadEdgesFromCSV(const FString& FilePath);

	// 棋子移动完成回调
	UFUNCTION()
	void OnPawnMoveComplete();

	// 移动完成后执行迷雾更新和节点事件
	void HandlePostMove(int32 TargetNodeID);

	// 缓存的目标节点 ID（棋子移动期间保存）
	int32 PendingMoveTargetNodeID = -1;
};
