// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "GameplayTagContainer.h"
#include "LFPSkillSelectionWidget.generated.h"

class ULFPSkillBase;
class ALFPTacticsUnit;
class ULFPSkillButtonWidget;
/**
 * 
 */
 // 技能排序方法枚举
UENUM(BlueprintType)
enum class ESkillSortMethod : uint8
{
    ByName,     // 按名称排序
    ByCooldown, // 按冷却时间排序
    ByCost,     // 按消耗排序
    ByType      // 按类型排序
};

// 技能选择委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillChosenSignature, ULFPSkillBase*, SelectedSkill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillConfirmedSignature, ULFPSkillBase*, SelectedSkill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSelectionCanceledSignature);

UCLASS()
class LFP2D_API ULFPSkillSelectionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // 构造函数
    ULFPSkillSelectionWidget(const FObjectInitializer& ObjectInitializer);

    // 初始化技能列表
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void InitializeSkills(ALFPTacticsUnit* Unit);

    // 显示技能选择窗口
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void Show();

    // 隐藏技能选择窗口
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void Hide();

    // 设置技能过滤器
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void SetSkillFilter(const FGameplayTagContainer& FilterTags);

    // 清除技能过滤器
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void ClearSkillFilter();

    // 排序技能
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void SortSkills(ESkillSortMethod SortMethod);

    // 刷新技能按钮状态
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void RefreshSkillButtons();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    // 技能选择事件处理
    UFUNCTION()
    void OnSkillSelected(ULFPSkillBase* Skill);

    // 技能悬停事件处理
    UFUNCTION()
    void OnSkillHovered(ULFPSkillBase* Skill);

    // 技能取消悬停事件处理
    UFUNCTION()
    void OnSkillUnhovered();

    // 确认按钮点击事件
    UFUNCTION()
    void OnConfirmClicked();

    // 取消按钮点击事件
    UFUNCTION()
    void OnCancelClicked();

    // 更新技能详情显示
    UFUNCTION()
    void UpdateSkillDetails(ULFPSkillBase* Skill);

    // 更新单位信息显示
    UFUNCTION()
    void UpdateUnitInfo();

    // 更新确认按钮状态
    UFUNCTION()
    void UpdateConfirmButtonState();

    // 获取过滤后的技能列表
    UFUNCTION()
    TArray<ULFPSkillBase*> GetFilteredSkills(ALFPTacticsUnit* Unit) const;

public:
    // 委托：技能被选中（但未确认）
    UPROPERTY(BlueprintAssignable, Category = "Skill Selection")
    FSkillChosenSignature OnSkillChosen;

    // 委托：技能被确认选择
    UPROPERTY(BlueprintAssignable, Category = "Skill Selection")
    FSkillConfirmedSignature OnSkillConfirmed;

    // 委托：选择被取消
    UPROPERTY(BlueprintAssignable, Category = "Skill Selection")
    FSelectionCanceledSignature OnSelectionCanceled;

protected:
    // 技能网格容器
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UUniformGridPanel* SkillGrid;

    // 技能详情面板
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UWidget* SkillDetailsPanel;

    // 技能名称文本
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillNameText;

    // 技能描述文本
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillDescriptionText;

    // 技能范围文本
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillRangeText;

    // 技能冷却文本
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillCooldownText;

    //// 技能图标
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UImage* SkillIcon;

    //// 单位名称文本
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* UnitNameText;

    //// 行动点文本
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* ActionPointsText;

    //// 血量文本
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* HealthText;

    //// 确认按钮
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UButton* ConfirmButton;

    //// 取消按钮
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UButton* CancelButton;

    //// 确认提示文本
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* ConfirmHintText;

    //// 无技能提示文本
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* NoSkillsText;

    // 技能按钮类
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    TSubclassOf<ULFPSkillButtonWidget> SkillButtonClass;

    //// 悬停音效
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* HoverSound;

    //// 选择音效
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* SelectSound;

    //// 确认音效
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* ConfirmSound;

    //// 取消音效
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* CancelSound;

private:
    // 当前选中的技能
    UPROPERTY()
    ULFPSkillBase* SelectedSkill;

    // 所属单位
    UPROPERTY()
    ALFPTacticsUnit* OwnerUnit;

    // 所有技能按钮
    UPROPERTY()
    TArray<ULFPSkillButtonWidget*> SkillButtons;

    // 技能过滤标签
    UPROPERTY()
    FGameplayTagContainer SkillFilterTags;

    // 最大列数
    UPROPERTY(EditAnywhere, Category = "Skill Selection")
    int32 MaxColumns;
};
