// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPTurnSpeedListWidget.generated.h"

class UHorizontalBox;
class UImage;

/**
 * 
 */
UCLASS()
class LFP2D_API ULFPTurnSpeedListWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void InitializeTurnOrder();

	// 初始化回合顺序

	// 更新单位顺序列表
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void UpdateTurnOrder();

	UFUNCTION()
	void OnTurnChanged();

	// 阶段变化响应
	UFUNCTION()
	void OnPhaseChanged(EBattlePhase NewPhase);

protected:
	// 显示回合数的文本

	// 阶段文本（可选绑定）
	// 用于存放单位头像的水平框
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* UnitIconsContainer;
	// 单位头像控件蓝图类（用于生成头像）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn UI")
	TSubclassOf<UUserWidget> UnitIconClass;

	UPROPERTY(BlueprintReadOnly, Category = "Turn Order")
	ALFPTurnManager* TurnManagerRef;

	FTimerHandle UpdateTimerHandle;
};
