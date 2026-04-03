#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/WorldMap/LFPWorldMapData.h"
#include "LFPWorldMapNode.generated.h"

class UPaperSpriteComponent;
class UPaperSprite;
class ULFPWorldNodeDataAsset;

UCLASS()
class LFP2D_API ALFPWorldMapNode : public AActor
{
	GENERATED_BODY()

public:
	ALFPWorldMapNode();

	// ============== 节点数据 ==============

	// 节点 ID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Node")
	int32 NodeID = -1;

	// 节点类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node")
	ELFPWorldNodeType NodeType = ELFPWorldNodeType::WNT_Battle;

	// ==== 战斗参数 ====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	FString BattleMapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle", meta = (ClampMin = "1", ClampMax = "5"))
	int32 StarRating = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	bool bCanEscape = true;

	// 基础金币奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	int32 BaseGoldReward = 0;

	// 基础食物奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Battle")
	int32 BaseFoodReward = 0;

	// ==== 事件参数 ====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Event")
	FString EventID;

	// ==== 城镇参数 ====

	// 城镇建筑列表字符串（分号分隔）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Town")
	FString TownBuildingList;

	// 商店配置 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Shop")
	FName ShopID = NAME_None;

	// 雇佣市场配置 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|HireMarket")
	FName HireMarketID = NAME_None;

	// ==== 解锁条件 ====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node")
	FString PrerequisiteNodeIDs;

	// ============== 运行时状态 ==============

	// 是否已触发过（战斗/事件节点首次触发后标记）
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "World Node|Runtime")
	bool bHasBeenTriggered = false;

	// 是否被迷雾覆盖
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "World Node|Runtime")
	bool bIsRevealed = false;

	// ============== 数据转换 ==============

	// 从 CSV 行数据初始化
	void InitFromRowData(const FLFPWorldNodeRow& Row);

	// 导出为 CSV 行数据
	FLFPWorldNodeRow ExportToRowData() const;

	// 获取 2D 位置
	UFUNCTION(BlueprintPure, Category = "World Node")
	FVector2D GetPosition2D() const;

	// 设置 2D 位置
	UFUNCTION(BlueprintCallable, Category = "World Node")
	void SetPosition2D(FVector2D NewPos);

	// ============== 视觉 ==============

	// 设置节点视觉数据
	UFUNCTION(BlueprintCallable, Category = "World Node|Visuals")
	void SetNodeVisualData(ULFPWorldNodeDataAsset* InData);

	// 根据状态更新精灵显示
	UFUNCTION(BlueprintCallable, Category = "World Node|Visuals")
	void UpdateVisualState();

	// 设置高亮
	UFUNCTION(BlueprintCallable, Category = "World Node|Visuals")
	void SetHighlighted(bool bHighlight);

protected:
	// 根组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	// 节点精灵
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> NodeSpriteComponent;

	// 高亮精灵
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> HighlightSpriteComponent;

	// 视觉数据资产引用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Node|Visuals")
	TObjectPtr<ULFPWorldNodeDataAsset> NodeVisualData;
};
