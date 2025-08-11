// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPTurnSpeedListWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UImage;

/**
 * 
 */
UCLASS()
class LFP2D_API ULFPTurnSpeedListWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 设置回合数
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void SetRoundNumber(int32 Round);
	// 更新行动单位列表
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void UpdateTurnOrder(const TArray<class ALFPTacticsUnit*>& Units, int32 CurrentUnitIndex);
protected:
	// 用于显示回合数的文本
	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoundText;
	// 用于放置单位头像的水平框
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* UnitIconsContainer;
	// 单位头像的控件蓝图类（在蓝图中设置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn UI")
	TSubclassOf<UUserWidget> UnitIconClass;
};
