// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Buff/LFPBuffRuntimeTypes.h"
#include "LFPBuffIconWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * 单个 Buff 图标的 C++ 基类。
 * 蓝图可继承此类，并放置 IconImage、StackText、TurnsText 这三个同名控件。
 */
UCLASS()
class LFP2D_API ULFPBuffIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 写入 UI 需要显示的 Buff 快照，并同步图标、层数、剩余回合等基础文本。
	UFUNCTION(BlueprintCallable, Category = "Buff|UI")
	void SetBuffEntry(const FLFPBuffDisplayEntry& Entry);

	// 当前图标正在显示的 Buff 数据。
	UPROPERTY(BlueprintReadOnly, Category = "Buff|UI")
	FLFPBuffDisplayEntry BuffEntry;

protected:
	// Buff 主图标。若 Buff 数据未配置 Icon，则保持蓝图中的默认外观。
	UPROPERTY(BlueprintReadOnly, Category = "Buff|UI", meta = (BindWidgetOptional))
	UImage* IconImage = nullptr;

	// 层数文本。StackCount <= 1 时会清空，适合叠层 Buff 显示右下角数字。
	UPROPERTY(BlueprintReadOnly, Category = "Buff|UI", meta = (BindWidgetOptional))
	UTextBlock* StackText = nullptr;

	// 剩余回合文本。RemainingTurns <= 0 时会清空，适合显示有限回合 Buff。
	UPROPERTY(BlueprintReadOnly, Category = "Buff|UI", meta = (BindWidgetOptional))
	UTextBlock* TurnsText = nullptr;

	// 蓝图扩展点：例如根据 Category 改边框颜色、根据 bIsActive 调整材质。
	UFUNCTION(BlueprintImplementableEvent, Category = "Buff|UI")
	void OnBuffEntryUpdated(const FLFPBuffDisplayEntry& Entry);
};
