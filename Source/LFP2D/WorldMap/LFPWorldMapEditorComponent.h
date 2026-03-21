#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/WorldMap/LFPWorldMapData.h"
#include "LFPWorldMapEditorComponent.generated.h"

class ALFPWorldMapManager;
class ALFPWorldMapNode;
class ALFPWorldMapEdge;

// 世界地图编辑器工具模式
UENUM(BlueprintType)
enum class ELFPWorldMapEditorTool : uint8
{
	WMET_None           UMETA(DisplayName = "无"),
	WMET_PlaceNode      UMETA(DisplayName = "放置节点"),
	WMET_RemoveNode     UMETA(DisplayName = "删除节点"),
	WMET_MoveNode       UMETA(DisplayName = "移动节点"),
	WMET_ConnectEdge    UMETA(DisplayName = "连接边"),
	WMET_RemoveEdge     UMETA(DisplayName = "删除边"),
	WMET_SetNodeParams  UMETA(DisplayName = "设置节点参数"),
	WMET_EditBattleMap  UMETA(DisplayName = "编辑战斗地图")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEditorToolChanged, ELFPWorldMapEditorTool, NewTool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEditorModeChanged, bool, bEditorActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEditorNodeSelected, ALFPWorldMapNode*, SelectedNode);

/**
 * 世界地图编辑器组件：管理编辑器状态、工具切换、笔刷参数
 * 挂载在 PlayerController 上
 */
UCLASS(Blueprintable, ClassGroup=(WorldMapEditor), meta=(BlueprintSpawnableComponent))
class LFP2D_API ULFPWorldMapEditorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULFPWorldMapEditorComponent();

	// 切换编辑器模式
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void ToggleEditorMode();

	UFUNCTION(BlueprintPure, Category = "World Map Editor")
	bool IsEditorActive() const { return bEditorActive; }

	// 设置工具
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void SetCurrentTool(ELFPWorldMapEditorTool Tool);

	UFUNCTION(BlueprintPure, Category = "World Map Editor")
	ELFPWorldMapEditorTool GetCurrentTool() const { return CurrentTool; }

	// ==== 笔刷参数 ====

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushNodeType(ELFPWorldNodeType InType) { BrushNodeType = InType; }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushStarRating(int32 InRating) { BrushStarRating = FMath::Clamp(InRating, 1, 5); }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushCanEscape(bool bInCanEscape) { BrushCanEscape = bInCanEscape; }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushBattleMapName(const FString& InName) { BrushBattleMapName = InName; }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushEventID(const FString& InID) { BrushEventID = InID; }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushEdgeTurnCost(int32 InCost) { BrushEdgeTurnCost = FMath::Max(1, InCost); }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushBaseGoldReward(int32 InGold) { BrushBaseGoldReward = FMath::Max(0, InGold); }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushBaseFoodReward(int32 InFood) { BrushBaseFoodReward = FMath::Max(0, InFood); }

	UFUNCTION(BlueprintCallable, Category = "World Map Editor|Brush")
	void SetBrushTownBuildingList(const FString& InList) { BrushTownBuildingList = InList; }

	// ==== 操作方法 ====

	// 在世界坐标处放置新节点
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	ALFPWorldMapNode* PlaceNodeAt(FVector2D WorldPos);

	// 选中已有节点（用于移动/连接/编辑参数）
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void SelectNode(ALFPWorldMapNode* Node);

	// 对选中节点应用参数（SetNodeParams 工具）
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void ApplyParamsToSelectedNode();

	// 开始连接边（第一次点击选中 FromNode，第二次点击选中 ToNode）
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void HandleEdgeConnection(ALFPWorldMapNode* Node);

	// 移动选中节点到新位置
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void MoveSelectedNodeTo(FVector2D NewPos);

	// 删除选中节点
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void RemoveSelectedNode();

	// 删除边（点击两个节点）
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void HandleEdgeRemoval(ALFPWorldMapNode* Node);

	// 进入战斗地图编辑（切换到内层编辑器）
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void EnterBattleMapEditor();

	// 从战斗地图编辑返回
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void ReturnFromBattleMapEditor();

	// 保存/加载
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	bool SaveWorldMap(const FString& MapName);

	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	bool LoadWorldMap(const FString& MapName);

	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	TArray<FString> GetSavedWorldMapList();

	// 新建空白世界地图
	UFUNCTION(BlueprintCallable, Category = "World Map Editor")
	void NewWorldMap();

	// ==== 委托 ====

	UPROPERTY(BlueprintAssignable, Category = "World Map Editor")
	FOnWorldEditorToolChanged OnToolChanged;

	UPROPERTY(BlueprintAssignable, Category = "World Map Editor")
	FOnWorldEditorModeChanged OnEditorModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "World Map Editor")
	FOnWorldEditorNodeSelected OnNodeSelected;

	// 获取选中节点
	UFUNCTION(BlueprintPure, Category = "World Map Editor")
	ALFPWorldMapNode* GetSelectedNode() const { return SelectedNode; }

	// 是否正在编辑战斗地图
	UFUNCTION(BlueprintPure, Category = "World Map Editor")
	bool IsEditingBattleMap() const { return bIsEditingBattleMap; }

protected:
	virtual void BeginPlay() override;

	ALFPWorldMapManager* GetWorldMapManager() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEditorActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ELFPWorldMapEditorTool CurrentTool = ELFPWorldMapEditorTool::WMET_None;

	// 笔刷参数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	ELFPWorldNodeType BrushNodeType = ELFPWorldNodeType::WNT_Battle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	int32 BrushStarRating = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	bool BrushCanEscape = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	FString BrushBattleMapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	FString BrushEventID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	int32 BrushEdgeTurnCost = 1;

	// 基础金币奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	int32 BrushBaseGoldReward = 0;

	// 基础食物奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	int32 BrushBaseFoodReward = 0;

	// 城镇建筑列表（分号分隔）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brush")
	FString BrushTownBuildingList;

	// 选中的节点（移动/连接/参数编辑用）
	UPROPERTY()
	TObjectPtr<ALFPWorldMapNode> SelectedNode;

	// 边连接的第一个节点（等待第二个节点）
	UPROPERTY()
	TObjectPtr<ALFPWorldMapNode> EdgeStartNode;

	// 边删除的第一个节点（等待第二个节点）
	UPROPERTY()
	TObjectPtr<ALFPWorldMapNode> EdgeRemovalStartNode;

	// 是否正在编辑内层战斗地图
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsEditingBattleMap = false;

	// 缓存
	UPROPERTY()
	mutable TObjectPtr<ALFPWorldMapManager> CachedWorldMapManager;
};
