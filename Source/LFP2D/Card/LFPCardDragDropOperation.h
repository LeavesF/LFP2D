#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPCardDragDropOperation.generated.h"

/* 卡牌拖拽操作：在拖拽过程中携带卡牌实例数据。 */
UCLASS()
class LFP2D_API ULFPCardDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Card Drag")
	FLFPCardInstance DraggedCard;
};
