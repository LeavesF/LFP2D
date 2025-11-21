// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "LFPSkillButtonWidget.generated.h"

// 前向声明
class ULFPSkillBase;
class ALFPTacticsUnit;

// 技能按钮委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillButtonClickedSignature, ULFPSkillBase*, Skill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillButtonHoveredSignature, ULFPSkillBase*, Skill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSkillButtonUnhoveredSignature);
/**
 * 
 */
UCLASS()
class LFP2D_API ULFPSkillButtonWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // 构造函数
    ULFPSkillButtonWidget(const FObjectInitializer& ObjectInitializer);

    // 初始化技能按钮
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void Initialize(ULFPSkillBase* Skill);

    // 刷新按钮状态
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void RefreshState();

    // 获取关联的技能
    UFUNCTION(BlueprintPure, Category = "Skill Button")
    ULFPSkillBase* GetSkill() const { return AssociatedSkill; }

    // 设置按钮是否可用
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void SetButtonEnabled(bool bEnabled);

    // 设置按钮选中状态
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void SetSelected(bool bSelected);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
    // 按钮点击事件处理
    UFUNCTION()
    void OnButtonClicked();

    // 更新按钮外观
    UFUNCTION()
    void UpdateAppearance();

    // 更新冷却显示
    UFUNCTION()
    void UpdateCooldownDisplay();

public:
    // 委托：按钮被点击
    UPROPERTY(BlueprintAssignable, Category = "Skill Button")
    FSkillButtonClickedSignature OnButtonClickedDelegate;

    // 委托：按钮被悬停
    UPROPERTY(BlueprintAssignable, Category = "Skill Button")
    FSkillButtonHoveredSignature OnButtonHoveredDelegate;

    // 委托：按钮取消悬停
    UPROPERTY(BlueprintAssignable, Category = "Skill Button")
    FSkillButtonUnhoveredSignature OnButtonUnhoveredDelegate;

public:
    // 按钮组件
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UButton* SkillButton;

    // 技能名称文本
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UTextBlock* SkillNameText;

    // 技能图标
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UImage* SkillIcon;

    // 冷却文本
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UTextBlock* CooldownText;

    // 消耗文本
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UTextBlock* CostText;

    // 选中高亮边框
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UImage* SelectionBorder;

    // 不可用遮罩
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UImage* DisabledOverlay;

    // 默认按钮样式
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FButtonStyle DefaultButtonStyle;

    // 选中按钮样式
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FButtonStyle SelectedButtonStyle;

    // 不可用按钮样式
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FButtonStyle DisabledButtonStyle;

    // 冷却中文本颜色
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FSlateColor CooldownTextColor;

    // 可用文本颜色
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FSlateColor AvailableTextColor;

    // 点击音效
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    USoundBase* ClickSound;

    // 悬停音效
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    USoundBase* HoverSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Attributes")
    ALFPTacticsUnit* OwnerUnit;

    // 关联的技能
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Attributes")
    ULFPSkillBase* AssociatedSkill;

    // 是否被选中
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Attributes")
    bool bIsSelected;
};
