#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPSaveSlotListItem.generated.h"

/**
 * 存档槽列表条目数据对象
 * 用于绑定到 SaveSlotListView (UListView)
 */
UCLASS(BlueprintType)
class LFP2D_API ULFPSaveSlotListItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FLFPSaveSlotInfo SlotInfo;
};
