// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPHealthBarWidget.generated.h"

class ALFPTacticsUnit;
class ULFPBuffIconWidget;

/**
 *
 */
UCLASS()
class LFP2D_API ULFPHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;

	// 绑定到单位
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void BindToUnit(ALFPTacticsUnit* Unit);

	// 解绑单位
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UnbindFromUnit();

	// 更新血条显示
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UpdateHealthBar(int32 CurrentHealth, int32 MaxHealth);

	// 重新读取当前单位的可见 Buff，并刷新血条下方的 Buff 图标。
	UFUNCTION(BlueprintCallable, Category = "Health Bar|Buff")
	void RefreshBuffIcons();

protected:
	// 血量变化事件处理
	UFUNCTION()
	void OnHealthChanged(int32 CurrentHealth, int32 MaxHealth);

	// 单位死亡事件处理
	UFUNCTION()
	void OnUnitDeath();

	// Buff 列表变化事件处理
	UFUNCTION()
	void OnBuffListChanged();

	UFUNCTION()
	void OnUnitAffiliationChanged(ALFPTacticsUnit* Unit, EUnitAffiliation OldAffiliation, EUnitAffiliation NewAffiliation);

	// 血条控件
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UProgressBar* HealthProgressBar;

	// 血量文本
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UTextBlock* HealthText;

	// 血条下方的 Buff 容器。蓝图中可使用 HorizontalBox、WrapBox 等任意 PanelWidget，并命名为 BuffContainer。
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar|Buff", meta = (BindWidgetOptional))
	UPanelWidget* BuffContainer;

	// 单个 Buff 图标 Widget 类。建议设置为继承自 LFPBuffIconWidget 的蓝图。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar|Buff")
	TSubclassOf<ULFPBuffIconWidget> BuffIconWidgetClass;

	// 最多显示的 Buff 图标数量，避免单位身上 Buff 太多时遮挡战场。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar|Buff", meta = (ClampMin = "0"))
	int32 MaxBuffIcons = 6;

	// 绑定的单位
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	ALFPTacticsUnit* BoundUnit;

	// 血条颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor FullHealthColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor LowHealthColor = FLinearColor::Red;

	// 低血量阈值（百分比）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHealthThreshold = 0.3f;

	// 阵营颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor PlayerColor = FLinearColor(0.1f, 1.0f, 0.1f); // 绿色

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor EnemyColor = FLinearColor(1.0f, 0.1f, 0.1f); // 红色

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor NeutralColor = FLinearColor(0.5f, 0.5f, 0.5f); // 灰色

	// 边框图片
	/*UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UImage* BorderImage;*/

	// 更新阵营颜色
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UpdateAffiliationColor(EUnitAffiliation Affiliation);
};
