// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPPlannedSkillIconWidget.generated.h"

class UImage;

/**
 * 头顶显示的计划技能图标 Widget
 */
UCLASS()
class LFP2D_API ULFPPlannedSkillIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    // 设置技能图标纹理
    UFUNCTION(BlueprintCallable, Category = "Planned Skill")
    void SetSkillIcon(UTexture2D* IconTexture);

protected:
    // 技能图标图片（需要在 UMG 蓝图中绑定）
    UPROPERTY(BlueprintReadOnly, Category = "Planned Skill", meta = (BindWidget))
    UImage* SkillIconImage;
};
