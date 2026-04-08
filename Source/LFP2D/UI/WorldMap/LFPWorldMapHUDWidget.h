#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPWorldMapHUDWidget.generated.h"

class ULFPGameInstance;
class ULFPWorldMapPlayerState;
class UTextBlock;
class UWrapBox;
class UImage;

/**
 * 世界地图 HUD Widget
 * 常驻显示金币、食物、回合天数、已拥有遗物图标
 * 布局完全由蓝图决定，代码只绑定控件
 */
UCLASS()
class LFP2D_API ULFPWorldMapHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化：传入数据源
	UFUNCTION(BlueprintCallable, Category = "WorldMap HUD")
	void Setup(ULFPGameInstance* GI, ULFPWorldMapPlayerState* PS);

protected:
	virtual void NativeDestruct() override;

private:
	// 刷新方法
	void RefreshAll();
	void RefreshResources();
	void RefreshTurnInfo();
	void RefreshRelicList();

	// 委托回调
	UFUNCTION()
	void OnResourceChanged(int32 NewGold, int32 NewFood);

	UFUNCTION()
	void OnOwnedRelicsChanged();

	UFUNCTION()
	void OnTurnChanged(int32 CurrentTurn, int32 TurnBudget);

	// 解绑所有委托
	void UnbindDelegates();

protected:
	// BindWidget 控件（全部 Optional，布局由蓝图决定）

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Gold;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Food;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Turn;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> Box_RelicList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic")
	float RelicIconSize = 60.f;

private:
	// 缓存数据源
	UPROPERTY()
	TObjectPtr<ULFPGameInstance> CachedGI;

	UPROPERTY()
	TObjectPtr<ULFPWorldMapPlayerState> CachedPS;
};
