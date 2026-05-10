// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/UI/Fighting/LFPSkillButtonWidget.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/UI/Fighting/MessageBox/LFPMessageBoxWidget.h"
#include "Sound/SoundBase.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "Kismet/GameplayStatics.h"

ULFPSkillButtonWidget::ULFPSkillButtonWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AssociatedSkill = nullptr;
    bIsSelected = false;
    bCanClickSkillButton = false;
    bNormalBrushCached = false;
    MessageBoxWidget = nullptr;

    // 默认颜色设置
    CooldownTextColor = FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f)); // 红色
    AvailableTextColor = FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f)); // 绿色
}

void ULFPSkillButtonWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 确保控件已绑定
    if (SkillButton)
    {
        // 绑定点击事件
        SkillButton->OnClicked.AddDynamic(this, &ULFPSkillButtonWidget::OnButtonClicked);
    }
}

void ULFPSkillButtonWidget::NativeDestruct()
{
    // 清理消息框
    if (MessageBoxWidget)
    {
        MessageBoxWidget->RemoveFromParent();
        MessageBoxWidget = nullptr;
    }

    Super::NativeDestruct();
}

void ULFPSkillButtonWidget::InitializeSkillButton(ULFPSkillBase* Skill)
{
    if (!Skill) return;

    AssociatedSkill = Skill;

    // 更新按钮显示
    UpdateAppearance();
}

void ULFPSkillButtonWidget::RefreshState()
{
    // 更新按钮显示
    UpdateAppearance();
}

void ULFPSkillButtonWidget::SetButtonEnabled(bool bEnabled)
{
    bCanClickSkillButton = bEnabled;

    // 控件和内部按钮保持启用，避免 UE disabled 状态影响图标渲染；
    // 点击是否生效由 bCanClickSkillButton 控制。
    SetIsEnabled(true);

    if (SkillButton)
    {
        SkillButton->SetIsEnabled(true);
    }

    // 更新禁用遮罩显示
    if (DisabledOverlay)
    {
        DisabledOverlay->SetVisibility(bEnabled ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
    }

    // 重新启用时恢复选中外观
    if (bEnabled && bIsSelected)
    {
        SetSelected(true);
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

    // 选中时让按钮保持Hovered外观
    if (SkillButton && GetIsEnabled())
    {
        FButtonStyle CurrentStyle = SkillButton->WidgetStyle;
        if (bSelected)
        {
            // 缓存原始Normal画刷，然后替换为Hovered画刷
            if (!bNormalBrushCached)
            {
                CachedNormalBrush = CurrentStyle.Normal;
                bNormalBrushCached = true;
            }
            CurrentStyle.Normal = CurrentStyle.Hovered;
        }
        else
        {
            // 恢复原始Normal画刷
            if (bNormalBrushCached)
            {
                CurrentStyle.Normal = CachedNormalBrush;
                bNormalBrushCached = false;
            }
        }
        SkillButton->SetStyle(CurrentStyle);
    }
}

void ULFPSkillButtonWidget::OnButtonClicked()
{
    if (!AssociatedSkill || !OwnerUnit || !bCanClickSkillButton) return;
    if (AssociatedSkill->IsPassiveSkill()) return;

    if (OnButtonClickedDelegate.IsBound())
    {
        OnButtonClickedDelegate.Broadcast(AssociatedSkill);
    }

    if (AssociatedSkill->TargetType == ESkillTargetType::Self)
    {
        TacticsPC->ExecuteSkill(AssociatedSkill);
        return;
    }

    TacticsPC->HandleSkillTargetSelecting(AssociatedSkill);
    //// 播放点击音效
    //if (ClickSound)
    //{
    //    UGameplayStatics::PlaySound2D(this, ClickSound);
    //}
}

void ULFPSkillButtonWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    // 播放悬停音效
    if (HoverSound && bCanClickSkillButton)
    {
        UGameplayStatics::PlaySound2D(this, HoverSound);
    }

    // 广播悬停委托
    if (OnButtonHoveredDelegate.IsBound())
    {
        OnButtonHoveredDelegate.Broadcast(AssociatedSkill);
    }

    // 悬停延迟后显示技能描述（延迟时间在 MessageBox 蓝图中配置）
    if (AssociatedSkill && !AssociatedSkill->SkillDescription.IsEmpty())
    {
        if (!MessageBoxWidget && MessageBoxClass)
        {
            MessageBoxWidget = CreateWidget<ULFPMessageBoxWidget>(GetOwningPlayer(), MessageBoxClass);
        }
        if (MessageBoxWidget)
        {
            MessageBoxWidget->RequestShow(AssociatedSkill->SkillDescription);
        }
    }
}

void ULFPSkillButtonWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    // 广播取消悬停委托
    if (OnButtonUnhoveredDelegate.IsBound())
    {
        OnButtonUnhoveredDelegate.Broadcast();
    }

    // 取消悬停并隐藏消息框
    if (MessageBoxWidget)
    {
        MessageBoxWidget->CancelRequest();
    }
}

void ULFPSkillButtonWidget::UpdateAppearance()
{
    if (!AssociatedSkill)
    {
        SetVisibility(ESlateVisibility::Hidden);
        return;
    }
    SetVisibility(ESlateVisibility::Visible);

    // 更新技能名称
    if (SkillNameText)
    {
        SkillNameText->SetText(AssociatedSkill->SkillName);
        //SkillNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    // 更新技能图标
    if (SkillIcon && AssociatedSkill->SkillIcon)
    {
        SkillIcon->SetBrushFromTexture(AssociatedSkill->SkillIcon);
        SkillIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
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
        const FString CostString = AssociatedSkill->IsPassiveSkill()
            ? FString(TEXT("被动"))
            : FString::Printf(TEXT("%d AP"), AssociatedSkill->ActionPointCost);
        CostText->SetText(FText::FromString(CostString));
        CostText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    // 更新按钮可用状态
    //bool bCanExecute = AssociatedSkill->CanExecute(nullptr); // 传入nullptr，因为我们不检查具体单位
    //SetButtonEnabled(bCanExecute);
    if (AssociatedSkill->IsPassiveSkill() && AssociatedSkill->ShouldShowDisabledInSkillBar())
    {
        SetButtonEnabled(false);
    }
    else
    {
        const bool bCanClick = AssociatedSkill->TargetType == ESkillTargetType::Self
            ? AssociatedSkill->CanExecute(nullptr)
            : AssociatedSkill->IsAvailable();
        SetButtonEnabled(bCanClick);
    }
}

void ULFPSkillButtonWidget::UpdateCooldownDisplay()
{
    if (!CooldownText || !AssociatedSkill) return;

    if (AssociatedSkill->IsPassiveSkill())
    {
        CooldownText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (AssociatedSkill->CurrentCooldown > 0)
    {
        // 显示冷却信息
        FString CooldownString = FString::Printf(TEXT("%d"), AssociatedSkill->CurrentCooldown);
        CooldownText->SetText(FText::FromString(CooldownString));
        CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
        CooldownText->SetColorAndOpacity(CooldownTextColor);
    }
    else if (AssociatedSkill->CooldownRounds > 0)
    {
        // 显示冷却回合数（当前不在冷却中）
        FString CooldownString = FString::Printf(TEXT("%dR"), AssociatedSkill->CooldownRounds);
        CooldownText->SetText(FText::FromString(CooldownString));
        CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
        CooldownText->SetColorAndOpacity(AvailableTextColor);
    }
    else
    {
        // 无冷却，隐藏冷却文本
        CooldownText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

