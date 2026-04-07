#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPTeleportWidget.generated.h"

class ALFPWorldMapNode;
class UButton;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeleportWidgetClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeleportTargetSelected, int32, TargetNodeID);

/**
 * 传送阵 UI Widget
 * 显示可传送的城镇节点列表，玩家选择后广播目标节点 ID
 */
UCLASS()
class LFP2D_API ULFPTeleportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 设置可传送目标列表
	UFUNCTION(BlueprintCallable, Category = "Teleport")
	void Setup(const TArray<ALFPWorldMapNode*>& TargetNodes);

	UPROPERTY(BlueprintAssignable)
	FOnTeleportWidgetClosed OnClosed;

	UPROPERTY(BlueprintAssignable)
	FOnTeleportTargetSelected OnTeleportTargetSelected;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void OnDestinationClicked();
	UFUNCTION() void OnCloseClicked();

protected:
	// 目标列表容器
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_DestinationList;

	// 关闭按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	// 标题
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

private:
	// 按钮 → 节点 ID 映射
	UPROPERTY()
	TArray<TObjectPtr<UButton>> DestinationButtons;

	TMap<UButton*, int32> ButtonToNodeIDMap;
};
