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

    //// 确保组件已绑定
    //if (!SkillGrid || !UnitNameText || !ActionPointsText)
    //{
    //    UE_LOG(LogTemp, Error, TEXT("Skill selection widget components not properly bound!"));
    //    return;
    //}

    // 初始隐藏详情面板
    if (SkillDetailsPanel)
    {
        SkillDetailsPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ULFPSkillSelectionWidget::InitializeSkills(ALFPTacticsUnit* Unit, ALFPTacticsPlayerController* PC)
{
    if (!Unit || !PC || !SkillGrid) return;

    OwnerUnit = Unit;
    TacticsPC = PC;

    // 清空现有技能按钮
    SkillGrid->ClearChildren();
    SkillButtons.Empty();

    // 更新单位信息
    UpdateUnitInfo();

    // 获取可用技能
    TArray<ULFPSkillBase*> AvailableSkills = Unit->GetAvailableSkills();

    // 创建技能按钮
    int32 Row = 0;
    int32 Column = 0;

    for (ULFPSkillBase* Skill : AvailableSkills)
    {
        if (!Skill) continue;

        // 创建技能按钮
        if (SkillButtonClass)
        {
            ULFPSkillButtonWidget* SkillButton = CreateWidget<ULFPSkillButtonWidget>(this, SkillButtonClass);
            if (SkillButton)
            {
                // 初始化按钮
                SkillButton->Initialize(Skill);
                SkillButton->OwnerUnit = OwnerUnit;
                SkillButton->TacticsPC = TacticsPC;
                // 绑定点击事件
                //SkillButton->OnSkillSelected.AddDynamic(this, &ULFPSkillSelectionWidget::OnSkillSelected);

                // 添加到网格
                SkillGrid->AddChildToUniformGrid(SkillButton, Row, Column);
                SkillButtons.Add(SkillButton);

                // 更新行列位置
                Column++;
                if (Column >= MaxColumns)
                {
                    Column = 0;
                    Row++;
                }
            }
        }
    }

    //// 如果没有可用技能，显示提示
    //if (AvailableSkills.Num() == 0)
    //{
    //    if (NoSkillsText)
    //    {
    //        NoSkillsText->SetVisibility(ESlateVisibility::Visible);
    //        NoSkillsText->SetText(FText::FromString("没有可用技能"));
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

    // 更新技能详情
    UpdateSkillDetails(Skill);

    // 显示详情面板
    if (SkillDetailsPanel)
    {
        SkillDetailsPanel->SetVisibility(ESlateVisibility::Visible);
    }

    // 通知外部技能被选中
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

    // 更新技能信息
    SkillNameText->SetText(Skill->SkillName);
    SkillDescriptionText->SetText(Skill->SkillDescription);

    //// 设置技能图标
    //if (Skill->SkillIcon)
    //{
    //    SkillIcon->SetBrushFromTexture(Skill->SkillIcon);
    //    SkillIcon->SetVisibility(ESlateVisibility::Visible);
    //}
    //else
    //{
    //    SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
    //}

    // 更新技能范围信息
    FString RangeText = FString::Printf(TEXT("范围: %d-%d"),
        Skill->Range.MinRange, Skill->Range.MaxRange);

    if (Skill->Range.bRequireLineOfSight)
    {
        RangeText += TEXT(" (需要视线)");
    }

    SkillRangeText->SetText(FText::FromString(RangeText));

    // 更新冷却信息
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

    // 更新行动点消耗
    FString APText = FString::Printf(TEXT("Consume: %dAP"), Skill->ActionPointCost);

    // 更新确认按钮状态
    UpdateConfirmButtonState();
}

void ULFPSkillSelectionWidget::UpdateUnitInfo()
{
    //if (!OwnerUnit || !UnitNameText || !ActionPointsText || !HealthText) return;

    //// 更新单位名称
    //UnitNameText->SetText(FText::FromString(OwnerUnit->GetName()));

    //// 更新行动点
    //FString APText = FString::Printf(TEXT("AP: %d/%d"),
    //    OwnerUnit->GetCurrentMovePoints(), OwnerUnit->GetMaxMovePoints());
    //ActionPointsText->SetText(FText::FromString(APText));

    //// 更新血量
    //FString HealthTextStr = FString::Printf(TEXT("HP: %d/%d"),
    //    OwnerUnit->GetCurrentHealth(), OwnerUnit->GetMaxHealth());
    //HealthText->SetText(FText::FromString(HealthTextStr));
}

void ULFPSkillSelectionWidget::UpdateConfirmButtonState()
{
    if (!OwnerUnit || !SelectedSkill) return;

    // 检查技能是否可用
    bool bCanExecute = SelectedSkill->CanExecute(OwnerUnit);

    //// 更新确认按钮状态
    //ConfirmButton->SetIsEnabled(bCanExecute);

    //// 更新提示文本
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

    // 检查技能是否可用
    if (!SelectedSkill->CanExecute(OwnerUnit))
    {
        // 播放错误音效或提示
        return;
    }

    // 通知外部确认技能选择
    if (OnSkillConfirmed.IsBound())
    {
        OnSkillConfirmed.Broadcast(SelectedSkill);
    }

    // 关闭窗口
    RemoveFromParent();
}

void ULFPSkillSelectionWidget::OnCancelClicked()
{
    // 通知外部取消技能选择
    if (OnSelectionCanceled.IsBound())
    {
        OnSelectionCanceled.Broadcast();
    }

    // 关闭窗口
    RemoveFromParent();
}

void ULFPSkillSelectionWidget::RefreshSkillButtons()
{
    if (!OwnerUnit) return;

    // 更新单位信息
    UpdateUnitInfo();

    // 更新所有技能按钮状态
    for (ULFPSkillButtonWidget* Button : SkillButtons)
    {
        if (Button)
        {
            Button->RefreshState();
        }
    }

    // 如果当前有选中的技能，更新其详情
    if (SelectedSkill)
    {
        UpdateSkillDetails(SelectedSkill);
    }
}

FReply ULFPSkillSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 处理键盘输入
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

    // 定期刷新技能按钮状态（可选，根据需要调整频率）
    static float RefreshTimer = 0.0f;
    RefreshTimer += InDeltaTime;

    if (RefreshTimer >= 0.5f) // 每0.5秒刷新一次
    {
        RefreshTimer = 0.0f;
        RefreshSkillButtons();
    }
}

void ULFPSkillSelectionWidget::Show()
{
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    //// 获取玩家控制器并设置输入模式
    //APlayerController* PC = GetOwningPlayer();
    //if (PC)
    //{
    //    // 设置UI-only输入模式
    //    FInputModeUIOnly InputMode;
    //    InputMode.SetWidgetToFocus(TakeWidget());
    //    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    //    PC->SetInputMode(InputMode);

    //    // 显示鼠标光标
    //    PC->bShowMouseCursor = true;
    //}
}

void ULFPSkillSelectionWidget::Hide()
{
    SetVisibility(ESlateVisibility::Collapsed);

    //// 恢复游戏输入模式
    //APlayerController* PC = GetOwningPlayer();
    //if (PC)
    //{
    //    FInputModeGameOnly InputMode;
    //    PC->SetInputMode(InputMode);

    //    // 隐藏鼠标光标
    //    PC->bShowMouseCursor = false;
    //}
}

void ULFPSkillSelectionWidget::SetSkillFilter(const FGameplayTagContainer& FilterTags)
{
    // 保存过滤条件
    SkillFilterTags = FilterTags;

    // 重新初始化技能列表
    if (OwnerUnit)
    {
        InitializeSkills(OwnerUnit, this);
    }
}

void ULFPSkillSelectionWidget::ClearSkillFilter()
{
    // 清空过滤条件
    SkillFilterTags.Reset();

    // 重新初始化技能列表
    if (OwnerUnit)
    {
        InitializeSkills(OwnerUnit, this);
    }
}

TArray<ULFPSkillBase*> ULFPSkillSelectionWidget::GetFilteredSkills(ALFPTacticsUnit* Unit) const
{
    TArray<ULFPSkillBase*> FilteredSkills;

    if (!Unit) return FilteredSkills;

    // 获取所有可用技能
    TArray<ULFPSkillBase*> AllSkills = Unit->GetAvailableSkills();

    // 如果没有过滤条件，返回所有技能
    if (SkillFilterTags.IsEmpty())
    {
        return AllSkills;
    }

    // 应用过滤条件
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

    // 获取当前显示的技能
    TArray<ULFPSkillBase*> CurrentSkills;
    for (ULFPSkillButtonWidget* Button : SkillButtons)
    {
        if (Button && Button->GetSkill())
        {
            CurrentSkills.Add(Button->GetSkill());
        }
    }

    // 根据排序方法排序
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
        // 需要技能类型属性，这里假设有SkillType属性
        // CurrentSkills.Sort([](const ULFPSkillBase& A, const ULFPSkillBase& B) {
        //     return A.SkillType < B.SkillType;
        // });
        break;
    }

    // 重新排列技能按钮
    if (SkillGrid)
    {
        SkillGrid->ClearChildren();

        int32 Row = 0;
        int32 Column = 0;

        for (ULFPSkillBase* Skill : CurrentSkills)
        {
            // 找到对应的按钮
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

    // 更新技能详情
    UpdateSkillDetails(Skill);

    // 显示详情面板
    if (SkillDetailsPanel)
    {
        SkillDetailsPanel->SetVisibility(ESlateVisibility::Visible);
    }

    //// 播放悬停音效
    //if (HoverSound)
    //{
    //    PlaySound(HoverSound);
    //}
}

void ULFPSkillSelectionWidget::OnSkillUnhovered()
{
    // 如果之前有选中的技能，显示选中技能的详情
    if (SelectedSkill)
    {
        UpdateSkillDetails(SelectedSkill);
    }
    else
    {
        // 隐藏详情面板
        if (SkillDetailsPanel)
        {
            SkillDetailsPanel->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}