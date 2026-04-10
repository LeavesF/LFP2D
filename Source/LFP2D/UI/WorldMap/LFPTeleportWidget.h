#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPTeleportWidget.generated.h"

class ALFPWorldMapNode;
class UButton;
class UTextBlock;
class UVerticalBox;
class ULFPTeleportWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeleportWidgetClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeleportTargetSelected, int32, TargetNodeID);

// 按钮点击路由对象，每个按钮持有一个，携带 NodeID
UCLASS()
class ULFPButtonClickRouter : public UObject
{
	GENERATED_BODY()

public:
	int32 NodeID = 0;

	UPROPERTY()
	TObjectPtr<ULFPTeleportWidget> OwnerWidget;

	UFUNCTION()
	void OnButtonClicked();
};

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

	// 由路由对象调用
	void HandleDestinationSelected(int32 NodeID);

	UPROPERTY(BlueprintAssignable)
	FOnTeleportWidgetClosed OnClosed;

	UPROPERTY(BlueprintAssignable)
	FOnTeleportTargetSelected OnTeleportTargetSelected;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void OnCloseClicked();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_DestinationList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

private:
	UPROPERTY()
	TArray<TObjectPtr<UButton>> DestinationButtons;

	// 路由对象，防止 GC 回收
	UPROPERTY()
	TArray<TObjectPtr<ULFPButtonClickRouter>> ClickRouters;
};
