#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/WorldMap/LFPWorldMapData.h"
#include "LFPWorldNodeDataAsset.generated.h"

class UPaperSprite;

/**
 * 世界地图节点视觉数据资产：定义每种节点类型的显示属性
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPWorldNodeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 节点类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Node")
	ELFPWorldNodeType NodeType = ELFPWorldNodeType::WNT_Battle;

	// 默认精灵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Node|Visuals")
	TObjectPtr<UPaperSprite> DefaultSprite = nullptr;

	// 已触发状态精灵（可选，空则用 DefaultSprite）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Node|Visuals")
	TObjectPtr<UPaperSprite> TriggeredSprite = nullptr;

	// 迷雾覆盖精灵（可选）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Node|Visuals")
	TObjectPtr<UPaperSprite> FogSprite = nullptr;

	// 显示名称（UI 用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Node|Visuals")
	FText DisplayName;
};
