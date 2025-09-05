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

    // Ĭ����ɫ����
    CooldownTextColor = FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f)); // ��ɫ
    AvailableTextColor = FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f)); // ��ɫ
}

void ULFPSkillButtonWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // ȷ������Ѱ�
    if (SkillButton)
    {
        // �󶨵���¼�
        SkillButton->OnClicked.AddDynamic(this, &ULFPSkillButtonWidget::OnButtonClicked);

        // Ӧ��Ĭ����ʽ
        SkillButton->SetStyle(DefaultButtonStyle);
    }

    // ��ʼ����ѡ�б߿�ͽ�������
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

    // ���°�ť��ʾ
    UpdateAppearance();
}

void ULFPSkillButtonWidget::RefreshState()
{
    if (!AssociatedSkill) return;

    // ���°�ť��ʾ
    UpdateAppearance();
}

void ULFPSkillButtonWidget::SetButtonEnabled(bool bEnabled)
{
    bIsEnabled = bEnabled;

    if (SkillButton)
    {
        SkillButton->SetIsEnabled(bEnabled);
    }

    // ���½���������ʾ
    if (DisabledOverlay)
    {
        DisabledOverlay->SetVisibility(bEnabled ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
    }

    // Ӧ����Ӧ�İ�ť��ʽ
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

    // ����ѡ�б߿���ʾ
    if (SelectionBorder)
    {
        SelectionBorder->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    // Ӧ����Ӧ�İ�ť��ʽ
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

    // ���ŵ����Ч
    if (ClickSound)
    {
        UGameplayStatics::PlaySound2D(this, ClickSound);
    }

    // �������ί��
    if (OnButtonClickedDelegate.IsBound())
    {
        OnButtonClickedDelegate.Broadcast(AssociatedSkill);
    }
}

void ULFPSkillButtonWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (!bIsEnabled) return;

    // ������ͣ��Ч
    if (HoverSound)
    {
        UGameplayStatics::PlaySound2D(this, HoverSound);
    }

    // ������ͣί��
    if (OnButtonHoveredDelegate.IsBound())
    {
        OnButtonHoveredDelegate.Broadcast(AssociatedSkill);
    }
}

void ULFPSkillButtonWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    // ����ȡ����ͣί��
    if (OnButtonUnhoveredDelegate.IsBound())
    {
        OnButtonUnhoveredDelegate.Broadcast();
    }
}

void ULFPSkillButtonWidget::UpdateAppearance()
{
    if (!AssociatedSkill) return;

    // ���¼�������
    if (SkillNameText)
    {
        SkillNameText->SetText(AssociatedSkill->SkillName);
    }

    // ���¼���ͼ��
    if (SkillIcon && AssociatedSkill->SkillIcon)
    {
        SkillIcon->SetBrushFromTexture(AssociatedSkill->SkillIcon);
        SkillIcon->SetVisibility(ESlateVisibility::Visible);
    }
    else if (SkillIcon)
    {
        SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
    }

    // ������ȴ��ʾ
    UpdateCooldownDisplay();

    // ����������ʾ
    if (CostText)
    {
        FString CostString = FString::Printf(TEXT("%d AP"), AssociatedSkill->ActionPointCost);
        CostText->SetText(FText::FromString(CostString));
    }

    // ���°�ť����״̬
    bool bCanExecute = AssociatedSkill->CanExecute(nullptr); // ����nullptr����Ϊ���ǲ������嵥λ
    SetButtonEnabled(bCanExecute);
}

void ULFPSkillButtonWidget::UpdateCooldownDisplay()
{
    if (!CooldownText || !AssociatedSkill) return;

    if (AssociatedSkill->CurrentCooldown > 0)
    {
        // ��ʾ��ȴ��Ϣ
        FString CooldownString = FString::Printf(TEXT("%d"), AssociatedSkill->CurrentCooldown);
        CooldownText->SetText(FText::FromString(CooldownString));
        CooldownText->SetVisibility(ESlateVisibility::Visible);
        CooldownText->SetColorAndOpacity(CooldownTextColor);
    }
    else if (AssociatedSkill->CooldownRounds > 0)
    {
        // ��ʾ��ȴ�غ���������ǰ������ȴ�У�
        FString CooldownString = FString::Printf(TEXT("%dR"), AssociatedSkill->CooldownRounds);
        CooldownText->SetText(FText::FromString(CooldownString));
        CooldownText->SetVisibility(ESlateVisibility::Visible);
        CooldownText->SetColorAndOpacity(AvailableTextColor);
    }
    else
    {
        // ����ȴ��������ȴ�ı�
        CooldownText->SetVisibility(ESlateVisibility::Collapsed);
    }
}