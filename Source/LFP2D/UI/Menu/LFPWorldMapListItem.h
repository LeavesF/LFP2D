#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LFPWorldMapListItem.generated.h"

/**
 * 世界地图列表条目数据对象
 * 用于绑定到 WorldMapListView (UListView)
 */
UCLASS(BlueprintType)
class LFP2D_API ULFPWorldMapListItem : public UObject
{
	GENERATED_BODY()

public:
	// 世界地图 CSV 文件名（不含后缀）
	UPROPERTY(BlueprintReadOnly, Category = "World Map")
	FString MapName;
};
