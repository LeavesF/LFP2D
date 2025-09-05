// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillButtonWidget.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

ULFPSkillButtonWidget::ULFPSkillButtonWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AssociatedSkill = nullptr;
    bIsSelected = false;
    bIsEnabled = true;

    // 默认颜色设置
    CooldownTextColor = FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f)); // 红色
    AvailableTextColor = FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f)); // 绿色
}

void ULFPSkillButtonWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 确保组件已绑定
    if (SkillButton)
    {
        // 绑定点击事件
        SkillButton->OnClicked.AddDynamic(this, &ULFPSkillButtonWidget::OnButtonClicked);

        // 应用默认样式
        SkillButton->SetStyle(DefaultButtonStyle);
    }

    // 初始隐藏选中边框和禁用遮罩
    if (SelectionBorder)
    {
        SelectionBorder->SetVisibility(ESlateVisibility::Hidden);
    }

    if (DisabledOverlay)
    {
        DisabledOverlay->SetVisibility(ESlateVisibility::Hidden);
    }
}

void ULFPSkillButtonWidget::Initialize(ULFPSkillBase* Skill)
{
    if (!Skill) return;

    AssociatedSkill = Skill;

    // 更新按钮显示
    UpdateAppearance();
}

void ULFPSkillButtonWidget::RefreshState()
{
    if (!AssociatedSkill) return;

    // 更新按钮显示
    UpdateAppearance();
}

void ULFPSkillButtonWidget::SetButtonEnabled(bool bEnabled)
{
    bIsEnabled = bEnabled;

    if (SkillButton)
    {
        SkillButton->SetIsEnabled(bEnabled);
    }

    // 更新禁用遮罩显示
    if (DisabledOverlay)
    {
        DisabledOverlay->SetVisibility(bEnabled ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
    }

    // 应用相应的按钮样式
    if (SkillButton)
    {
        if (!bEnabled)
        {
            SkillButton->SetStyle(DisabledButtonStyle);
        }
        else if (bIsSelected)
        {
            SkillButton->SetStyle(SelectedButtonStyle);
        }
        else
        {
            SkillButton->SetStyle(DefaultButtonStyle);
        }
    }
}

void ULFPSkillButtonWidget::SetSelected(bool bSelected)
{
    bIsSelected = bSelected;

    // 更新选中边框显示
    if (SelectionBorder)
    {
        SelectionBorder->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    // 应用相应的按钮样式
    if (SkillButton && bIsEnabled)
    {
        if (bSelected)
        {
            SkillButton->SetStyle(SelectedButtonStyle);
        }
        else
        {
            SkillButton->SetStyle(DefaultButtonStyle);
        }
    }
}

void ULFPSkillButtonWidget::OnButtonClicked()
{
    if (!AssociatedSkill || !bIsEnabled) return;

    // 播放点击音效
    if (ClickSound)
    {
        UGameplayStatics::PlaySound2D(this, ClickSound);
    }

    // 触发点击委托
    if (OnButtonClickedDelegate.IsBound())
    {
        OnButtonClickedDelegate.Broadcast(AssociatedSkill);
    }
}

void ULFPSkillButtonWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (!bIsEnabled) return;

    // 播放悬停音效
    if (HoverSound)
    {
        UGameplayStatics::PlaySound2D(this, HoverSound);
    }

    // 触发悬停委托
    if (OnButtonHoveredDelegate.IsBound())
    {
        OnButtonHoveredDelegate.Broadcast(AssociatedSkill);
    }
}

void ULFPSkillButtonWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    // 触发取消悬停委托
    if (OnButtonUnhoveredDelegate.IsBound())
    {
        OnButtonUnhoveredDelegate.Broadcast();
    }
}

void ULFPSkillButtonWidget::UpdateAppearance()
{
    if (!AssociatedSkill) return;

    // 更新技能名称
    if (SkillNameText)
    {
        SkillNameText->SetText(AssociatedSkill->SkillName);
    }

    // 更新技能图标
    if (SkillIcon && AssociatedSkill->SkillIcon)
    {
        SkillIcon->SetBrushFromTexture(AssociatedSkill->SkillIcon);
        SkillIcon->SetVisibility(ESlateVisibility::Visible);
    }
    else if (SkillIcon)
    {
        SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 更新冷却显示
    UpdateCooldownDisplay();

    // 更新消耗显示
    if (CostText)
    {
        FString CostString = FString::Printf(TEXT("%d AP"), AssociatedSkill->ActionPointCost);
        CostText->SetText(FText::FromString(CostString));
    }

    // 更新按钮可用状态
    bool bCanExecute = AssociatedSkill->CanExecute(nullptr); // 传入nullptr，因为我们不检查具体单位
    SetButtonEnabled(bCanExecute);
}

void ULFPSkillButtonWidget::UpdateCooldownDisplay()
{
    if (!CooldownText || !AssociatedSkill) return;

    if (AssociatedSkill->CurrentCooldown > 0)
    {
        // 显示冷却信息
        FString CooldownString = FString::Printf(TEXT("%d"), AssociatedSkill->CurrentCooldown);
        CooldownText->SetText(FText::FromString(CooldownString));
        CooldownText->SetVisibility(ESlateVisibility::Visible);
        CooldownText->SetColorAndOpacity(CooldownTextColor);
    }
    else if (AssociatedSkill->CooldownRounds > 0)
    {
        // 显示冷却回合数（但当前不在冷却中）
        FString CooldownString = FString::Printf(TEXT("%dR"), AssociatedSkill->CooldownRounds);
        CooldownText->SetText(FText::FromString(CooldownString));
        CooldownText->SetVisibility(ESlateVisibility::Visible);
        CooldownText->SetColorAndOpacity(AvailableTextColor);
    }
    else
    {
        // 无冷却，隐藏冷却文本
        CooldownText->SetVisibility(ESlateVisibility::Collapsed);
    }
}