#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/WorldMap/LFPWorldMapData.h"
#include "LFPWorldMapEdge.generated.h"

class UPaperSpriteComponent;
class UPaperSprite;

UCLASS()
class LFP2D_API ALFPWorldMapEdge : public AActor
{
	GENERATED_BODY()

public:
	ALFPWorldMapEdge();

	// ============== 边数据 ==============

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Edge")
	int32 FromNodeID = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Edge")
	int32 ToNodeID = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Edge", meta = (ClampMin = "1"))
	int32 TravelTurnCost = 1;

	// ============== 数据转换 ==============

	void InitFromRowData(const FLFPWorldEdgeRow& Row);
	FLFPWorldEdgeRow ExportToRowData() const;

	// ============== 视觉 ==============

	// 更新线段位置（根据两端节点坐标）
	UFUNCTION(BlueprintCallable, Category = "World Edge|Visuals")
	void UpdateVisualPosition(FVector StartPos, FVector EndPos);

	// 设置边精灵（拉伸的线段精灵）
	UFUNCTION(BlueprintCallable, Category = "World Edge|Visuals")
	void SetEdgeSprite(UPaperSprite* InSprite);

	// 设置高亮
	UFUNCTION(BlueprintCallable, Category = "World Edge|Visuals")
	void SetHighlighted(bool bHighlight);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	// 边精灵（拉伸的线段）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> EdgeSpriteComponent;

	// 边的两端世界坐标（缓存）
	FVector CachedStartPos;
	FVector CachedEndPos;
};
