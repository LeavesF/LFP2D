// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPHealthBarWidget.generated.h"

class ALFPTacticsUnit;

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

protected:
	// 血量变化事件处理
	UFUNCTION()
	void OnHealthChanged(int32 CurrentHealth, int32 MaxHealth);

	// 单位死亡事件处理
	UFUNCTION()
	void OnUnitDeath();

	// 血条控件
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UProgressBar* HealthProgressBar;

	// 血量文本
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UTextBlock* HealthText;

	// 所属单位
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	ALFPTacticsUnit* BoundUnit;

	// 血条颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor FullHealthColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor LowHealthColor = FLinearColor::Red;

	// 低血量阈值 (百分比)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHealthThreshold = 0.3f;

	// 阵营颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor PlayerColor = FLinearColor(0.1f, 1.0f, 0.1f); // 蓝色

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor EnemyColor = FLinearColor(1.0f, 0.1f, 0.1f); // 红色

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor NeutralColor = FLinearColor(0.5f, 0.5f, 0.5f); // 灰色

	// 边框图像
	/*UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UImage* BorderImage;*/

	// 更新阵营颜色
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UpdateAffiliationColor(EUnitAffiliation Affiliation);
};
