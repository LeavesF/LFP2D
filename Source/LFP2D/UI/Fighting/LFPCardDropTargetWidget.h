#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPCardDropTargetWidget.generated.h"

class UWidget;

UCLASS()
class LFP2D_API ULFPCardDropTargetWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Drop Target", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> NoTargetDropZone = nullptr;

public:
	bool IsViewportPositionInsideNoTargetZone(FVector2D ViewportPosition) const;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

private:
	bool IsDropInsideNoTargetZone(const FDragDropEvent& InDragDropEvent) const;
	bool IsSlatePositionInsideNoTargetZone(FVector2D SlatePosition) const;
};
