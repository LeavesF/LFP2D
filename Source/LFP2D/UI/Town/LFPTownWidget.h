#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Town/LFPTownData.h"
#include "LFPTownWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTownWidgetClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTownBuildingRequested, ELFPTownBuildingType, BuildingType);

/**
 * 城镇面板 UI Widget
 * 建筑按钮和图片在 UMG 蓝图中预先配置好布局
 * Setup 时根据城镇拥有的建筑列表控制各按钮的可见性
 */
UCLASS()
class LFP2D_API ULFPTownWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 根据城镇建筑类型列表，显示/隐藏对应按钮
	UFUNCTION(BlueprintCallable, Category = "Town")
	void Setup(const TArray<ELFPTownBuildingType>& AvailableBuildings);

	// 关闭委托
	UPROPERTY(BlueprintAssignable)
	FOnTownWidgetClosed OnClosed;

	// 建筑功能请求委托（由 PlayerController 监听并分发）
	UPROPERTY(BlueprintAssignable)
	FOnTownBuildingRequested OnBuildingRequested;

protected:
	virtual void NativeOnInitialized() override;

private:
	// 各建筑按钮点击回调
	UFUNCTION() void OnShopClicked();
	UFUNCTION() void OnEvolutionTowerClicked();
	UFUNCTION() void OnTeleportClicked();
	UFUNCTION() void OnQuestNPCClicked();
	UFUNCTION() void OnSkillNodeClicked();

	// 点击关闭
	UFUNCTION() void OnCloseClicked();

	// 隐藏所有建筑按钮
	void HideAllBuildings();

protected:
	// ==== BindWidget 控件（在 UMG 蓝图中预先配置好图片和布局） ====

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TownName;

	// 商店
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Shop;

	// 升华塔
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_EvolutionTower;

	// 传送阵
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Teleport;

	// 任务NPC
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_QuestNPC;

	// 技能节点
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SkillNode;

	// 关闭按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;
};
