#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPBattleResultWidget.generated.h"

class UTextBlock;
class UButton;
class UWrapBox;
class ULFPUnitRegistryDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleResultConfirmedSignature);

/**
 * 战斗结算面板：全屏显示胜负、奖励（金币/食物）、捕获单位
 */
UCLASS()
class LFP2D_API ULFPBattleResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化结算数据
	UFUNCTION(BlueprintCallable, Category = "Battle Result")
	void Setup(const FLFPBattleResult& Result, ULFPUnitRegistryDataAsset* Registry);

	// 确认按钮委托
	UPROPERTY(BlueprintAssignable)
	FOnBattleResultConfirmedSignature OnConfirmPressed;

protected:
	virtual void NativeConstruct() override;

	// ============== BindWidget ==============

	// 胜负标题文本
	UPROPERTY(BlueprintReadOnly, Category = "Battle Result", meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Result;

	// 金币奖励文本
	UPROPERTY(BlueprintReadOnly, Category = "Battle Result", meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Gold;

	// 食物奖励文本
	UPROPERTY(BlueprintReadOnly, Category = "Battle Result", meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Food;

	// 捕获单位容器（可选，胜利且有捕获时显示）
	UPROPERTY(BlueprintReadOnly, Category = "Battle Result", meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> Box_CapturedUnits;

	// 确认按钮
	UPROPERTY(BlueprintReadOnly, Category = "Battle Result", meta = (BindWidget))
	TObjectPtr<UButton> Button_Confirm;

	// ============== 数据缓存 ==============

	UPROPERTY()
	FLFPBattleResult CachedResult;

	UPROPERTY()
	TObjectPtr<ULFPUnitRegistryDataAsset> UnitRegistry;

private:
	UFUNCTION()
	void OnConfirmClicked();
};
