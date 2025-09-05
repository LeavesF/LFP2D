// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/UI/Fighting/LFPSkillSelectionWidget.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillButtonWidget.h"
#include "GameplayTagContainer.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

ULFPSkillSelectionWidget::ULFPSkillSelectionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SelectedSkill = nullptr;
    OwnerUnit = nullptr;
    MaxColumns = 4;
}

void ULFPSkillSelectionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //// ȷ������Ѱ�
    //if (!SkillGrid || !UnitNameText || !ActionPointsText)
    //{
    //    UE_LOG(LogTemp, Error, TEXT("Skill selection widget components not properly bound!"));
    //    return;
    //}

    // ��ʼ�����������
    if (SkillDetailsPanel)
    {
        SkillDetailsPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ULFPSkillSelectionWidget::InitializeSkills(ALFPTacticsUnit* Unit)
{
    if (!Unit || !SkillGrid) return;

    OwnerUnit = Unit;

    // ������м��ܰ�ť
    SkillGrid->ClearChildren();
    SkillButtons.Empty();

    // ���µ�λ��Ϣ
    UpdateUnitInfo();

    // ��ȡ���ü���
    TArray<ULFPSkillBase*> AvailableSkills = Unit->GetAvailableSkills();

    // �������ܰ�ť
    int32 Row = 0;
    int32 Column = 0;

    for (ULFPSkillBase* Skill : AvailableSkills)
    {
        if (!Skill) continue;

        // �������ܰ�ť
        if (SkillButtonClass)
        {
            ULFPSkillButtonWidget* SkillButton = CreateWidget<ULFPSkillButtonWidget>(this, SkillButtonClass);
            if (SkillButton)
            {
                // ��ʼ����ť
                SkillButton->Initialize(Skill);

                // �󶨵���¼�
                //SkillButton->OnSkillSelected.AddDynamic(this, &ULFPSkillSelectionWidget::OnSkillSelected);

                // ��ӵ�����
                SkillGrid->AddChildToUniformGrid(SkillButton, Row, Column);
                SkillButtons.Add(SkillButton);

                // ��������λ��
                Column++;
                if (Column >= MaxColumns)
                {
                    Column = 0;
                    Row++;
                }
            }
        }
    }

    //// ���û�п��ü��ܣ���ʾ��ʾ
    //if (AvailableSkills.Num() == 0)
    //{
    //    if (NoSkillsText)
    //    {
    //        NoSkillsText->SetVisibility(ESlateVisibility::Visible);
    //        NoSkillsText->SetText(FText::FromString("û�п��ü���"));
    //    }
    //}
    //else
    //{
    //    if (NoSkillsText)
    //    {
    //        NoSkillsText->SetVisibility(ESlateVisibility::Collapsed);
    //    }
    //}
}

void ULFPSkillSelectionWidget::OnSkillSelected(ULFPSkillBase* Skill)
{
    if (!Skill || !OwnerUnit) return;

    SelectedSkill = Skill;

    // ���¼�������
    UpdateSkillDetails(Skill);

    // ��ʾ�������
    if (SkillDetailsPanel)
    {
        SkillDetailsPanel->SetVisibility(ESlateVisibility::Visible);
    }

    // ֪ͨ�ⲿ���ܱ�ѡ��
    if (OnSkillChosen.IsBound())
    {
        OnSkillChosen.Broadcast(Skill);
    }
}

void ULFPSkillSelectionWidget::UpdateSkillDetails(ULFPSkillBase* Skill)
{
    if (!Skill || !SkillNameText || !SkillDescriptionText || !SkillRangeText ||
        !SkillCooldownText)
    {
        return;
    }

    // ���¼�����Ϣ
    SkillNameText->SetText(Skill->SkillName);
    SkillDescriptionText->SetText(Skill->SkillDescription);

    //// ���ü���ͼ��
    //if (Skill->SkillIcon)
    //{
    //    SkillIcon->SetBrushFromTexture(Skill->SkillIcon);
    //    SkillIcon->SetVisibility(ESlateVisibility::Visible);
    //}
    //else
    //{
    //    SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
    //}

    // ���¼��ܷ�Χ��Ϣ
    FString RangeText = FString::Printf(TEXT("��Χ: %d-%d"),
        Skill->Range.MinRange, Skill->Range.MaxRange);

    if (Skill->Range.bRequireLineOfSight)
    {
        RangeText += TEXT(" (��Ҫ����)");
    }

    SkillRangeText->SetText(FText::FromString(RangeText));

    // ������ȴ��Ϣ
    FString CooldownText;
    if (Skill->CooldownRounds > 0)
    {
        if (Skill->CurrentCooldown > 0)
        {
            CooldownText = FString::Printf(TEXT("CD: %d/%dRound"),
                Skill->CurrentCooldown, Skill->CooldownRounds);
        }
        else
        {
            CooldownText = FString::Printf(TEXT("CD: %dRound"), Skill->CooldownRounds);
        }
    }
    else
    {
        CooldownText = TEXT("No CD");
    }

    SkillCooldownText->SetText(FText::FromString(CooldownText));

    // �����ж�������
    FString APText = FString::Printf(TEXT("Consume: %dAP"), Skill->ActionPointCost);

    // ����ȷ�ϰ�ť״̬
    UpdateConfirmButtonState();
}

void ULFPSkillSelectionWidget::UpdateUnitInfo()
{
    //if (!OwnerUnit || !UnitNameText || !ActionPointsText || !HealthText) return;

    //// ���µ�λ����
    //UnitNameText->SetText(FText::FromString(OwnerUnit->GetName()));

    //// �����ж���
    //FString APText = FString::Printf(TEXT("AP: %d/%d"),
    //    OwnerUnit->GetCurrentMovePoints(), OwnerUnit->GetMaxMovePoints());
    //ActionPointsText->SetText(FText::FromString(APText));

    //// ����Ѫ��
    //FString HealthTextStr = FString::Printf(TEXT("HP: %d/%d"),
    //    OwnerUnit->GetCurrentHealth(), OwnerUnit->GetMaxHealth());
    //HealthText->SetText(FText::FromString(HealthTextStr));
}

void ULFPSkillSelectionWidget::UpdateConfirmButtonState()
{
    if (!OwnerUnit || !SelectedSkill) return;

    // ��鼼���Ƿ����
    bool bCanExecute = SelectedSkill->CanExecute(OwnerUnit);

    //// ����ȷ�ϰ�ť״̬
    //ConfirmButton->SetIsEnabled(bCanExecute);

    //// ������ʾ�ı�
    //if (ConfirmHintText)
    //{
    //    if (bCanExecute)
    //    {
    //        ConfirmHintText->SetText(FText::FromString("Click to confirm"));
    //        ConfirmHintText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
    //    }
    //    else
    //    {
    //        FString HintText;

    //        if (SelectedSkill->CurrentCooldown > 0)
    //        {
    //            HintText = FString::Printf(TEXT("In CD(%dRound)"), SelectedSkill->CurrentCooldown);
    //        }
    //        else if (!OwnerUnit->HasEnoughMovePoints(SelectedSkill->ActionPointCost))
    //        {
    //            HintText = FString::Printf(TEXT("Leak AP (need%d)"), SelectedSkill->ActionPointCost);
    //        }
    //        else
    //        {
    //            HintText = TEXT("Can not use this skill");
    //        }

    //        ConfirmHintText->SetText(FText::FromString(HintText));
    //        ConfirmHintText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    //    }
    //}
}

void ULFPSkillSelectionWidget::OnConfirmClicked()
{
    if (!SelectedSkill || !OwnerUnit) return;

    // ��鼼���Ƿ����
    if (!SelectedSkill->CanExecute(OwnerUnit))
    {
        // ���Ŵ�����Ч����ʾ
        return;
    }

    // ֪ͨ�ⲿȷ�ϼ���ѡ��
    if (OnSkillConfirmed.IsBound())
    {
        OnSkillConfirmed.Broadcast(SelectedSkill);
    }

    // �رմ���
    RemoveFromParent();
}

void ULFPSkillSelectionWidget::OnCancelClicked()
{
    // ֪ͨ�ⲿȡ������ѡ��
    if (OnSelectionCanceled.IsBound())
    {
        OnSelectionCanceled.Broadcast();
    }

    // �رմ���
    RemoveFromParent();
}

void ULFPSkillSelectionWidget::RefreshSkillButtons()
{
    if (!OwnerUnit) return;

    // ���µ�λ��Ϣ
    UpdateUnitInfo();

    // �������м��ܰ�ť״̬
    for (ULFPSkillButtonWidget* Button : SkillButtons)
    {
        if (Button)
        {
            Button->RefreshState();
        }
    }

    // �����ǰ��ѡ�еļ��ܣ�����������
    if (SelectedSkill)
    {
        UpdateSkillDetails(SelectedSkill);
    }
}

FReply ULFPSkillSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // �����������
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        OnCancelClicked();
        return FReply::Handled();
    }
    else if (InKeyEvent.GetKey() == EKeys::Enter || InKeyEvent.GetKey() == EKeys::SpaceBar)
    {
        OnConfirmClicked();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULFPSkillSelectionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // ����ˢ�¼��ܰ�ť״̬����ѡ��������Ҫ����Ƶ�ʣ�
    static float RefreshTimer = 0.0f;
    RefreshTimer += InDeltaTime;

    if (RefreshTimer >= 0.5f) // ÿ0.5��ˢ��һ��
    {
        RefreshTimer = 0.0f;
        RefreshSkillButtons();
    }
}

void ULFPSkillSelectionWidget::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    // ��ȡ��ҿ���������������ģʽ
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        // ����UI-only����ģʽ
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);

        // ��ʾ�����
        PC->bShowMouseCursor = true;
    }
}

void ULFPSkillSelectionWidget::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);

    // �ָ���Ϸ����ģʽ
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);

        // ���������
        PC->bShowMouseCursor = false;
    }
}

void ULFPSkillSelectionWidget::SetSkillFilter(const FGameplayTagContainer& FilterTags)
{
    // �����������
    SkillFilterTags = FilterTags;

    // ���³�ʼ�������б�
    if (OwnerUnit)
    {
        InitializeSkills(OwnerUnit);
    }
}

void ULFPSkillSelectionWidget::ClearSkillFilter()
{
    // ��չ�������
    SkillFilterTags.Reset();

    // ���³�ʼ�������б�
    if (OwnerUnit)
    {
        InitializeSkills(OwnerUnit);
    }
}

TArray<ULFPSkillBase*> ULFPSkillSelectionWidget::GetFilteredSkills(ALFPTacticsUnit* Unit) const
{
    TArray<ULFPSkillBase*> FilteredSkills;

    if (!Unit) return FilteredSkills;

    // ��ȡ���п��ü���
    TArray<ULFPSkillBase*> AllSkills = Unit->GetAvailableSkills();

    // ���û�й����������������м���
    if (SkillFilterTags.IsEmpty())
    {
        return AllSkills;
    }

    // Ӧ�ù�������
    for (ULFPSkillBase* Skill : AllSkills)
    {
        if (Skill && Skill->SkillTags.HasAny(SkillFilterTags))
        {
            FilteredSkills.Add(Skill);
        }
    }

    return FilteredSkills;
}

void ULFPSkillSelectionWidget::SortSkills(ESkillSortMethod SortMethod)
{
    if (!OwnerUnit) return;

    // ��ȡ��ǰ��ʾ�ļ���
    TArray<ULFPSkillBase*> CurrentSkills;
    for (ULFPSkillButtonWidget* Button : SkillButtons)
    {
        if (Button && Button->GetSkill())
        {
            CurrentSkills.Add(Button->GetSkill());
        }
    }

    // �������򷽷�����
    switch (SortMethod)
    {
    case ESkillSortMethod::ByName:
        CurrentSkills.Sort([](const ULFPSkillBase& A, const ULFPSkillBase& B) {
            return A.SkillName.ToString() < B.SkillName.ToString();
            });
        break;

    case ESkillSortMethod::ByCooldown:
        CurrentSkills.Sort([](const ULFPSkillBase& A, const ULFPSkillBase& B) {
            return A.CurrentCooldown < B.CurrentCooldown;
            });
        break;

    case ESkillSortMethod::ByCost:
        CurrentSkills.Sort([](const ULFPSkillBase& A, const ULFPSkillBase& B) {
            return A.ActionPointCost < B.ActionPointCost;
            });
        break;

    case ESkillSortMethod::ByType:
        // ��Ҫ�����������ԣ����������SkillType����
        // CurrentSkills.Sort([](const ULFPSkillBase& A, const ULFPSkillBase& B) {
        //     return A.SkillType < B.SkillType;
        // });
        break;
    }

    // �������м��ܰ�ť
    if (SkillGrid)
    {
        SkillGrid->ClearChildren();

        int32 Row = 0;
        int32 Column = 0;

        for (ULFPSkillBase* Skill : CurrentSkills)
        {
            // �ҵ���Ӧ�İ�ť
            for (ULFPSkillButtonWidget* Button : SkillButtons)
            {
                if (Button && Button->GetSkill() == Skill)
                {
                    SkillGrid->AddChildToUniformGrid(Button, Row, Column);

                    Column++;
                    if (Column >= MaxColumns)
                    {
                        Column = 0;
                        Row++;
                    }

                    break;
                }
            }
        }
    }
}

void ULFPSkillSelectionWidget::OnSkillHovered(ULFPSkillBase* Skill)
{
    if (!Skill) return;

    // ���¼�������
    UpdateSkillDetails(Skill);

    // ��ʾ�������
    if (SkillDetailsPanel)
    {
        SkillDetailsPanel->SetVisibility(ESlateVisibility::Visible);
    }

    //// ������ͣ��Ч
    //if (HoverSound)
    //{
    //    PlaySound(HoverSound);
    //}
}

void ULFPSkillSelectionWidget::OnSkillUnhovered()
{
    // ���֮ǰ��ѡ�еļ��ܣ���ʾѡ�м��ܵ�����
    if (SelectedSkill)
    {
        UpdateSkillDetails(SelectedSkill);
    }
    else
    {
        // �����������
        if (SkillDetailsPanel)
        {
            SkillDetailsPanel->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}