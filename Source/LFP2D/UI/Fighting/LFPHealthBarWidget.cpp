// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/UI/Fighting/LFPHealthBarWidget.h"

#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/UI/Fighting/LFPBuffIconWidget.h"
#include "LFP2D/UI/Fighting/LFPPlannedSkillIconWidget.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 确保控件已绑定
	if (HealthProgressBar && HealthText)
	{
		// 初始化满血条
		//SetVisibility(ESlateVisibility::Hidden);
	}

	HidePlannedSkillIcon();
}

void ULFPHealthBarWidget::BindToUnit(ALFPTacticsUnit* Unit)
{
	if (!Unit || BoundUnit == Unit) return;

	// 解绑旧单位
	UnbindFromUnit();

	// 绑定新单位
	BoundUnit = Unit;

	// 订阅事件
	BoundUnit->OnHealthChangedDelegate.AddDynamic(this, &ULFPHealthBarWidget::OnHealthChanged);
	BoundUnit->OnDeathDelegate.AddDynamic(this, &ULFPHealthBarWidget::OnUnitDeath);
	BoundUnit->OnAffiliationChangedDelegate.AddDynamic(this, &ULFPHealthBarWidget::OnUnitAffiliationChanged);
	if (ULFPBuffComponent* BuffComponent = BoundUnit->GetBuffComponent())
	{
		BuffComponent->OnBuffListChanged.AddDynamic(this, &ULFPHealthBarWidget::OnBuffListChanged);
	}

	// 初始化数据
	UpdateHealthBar(BoundUnit->GetCurrentHealth(), BoundUnit->GetMaxHealth());
	RefreshBuffIcons();

	// 显示血条
	SetVisibility(ESlateVisibility::Visible);

	// 更新阵营颜色
	if (BoundUnit)
	{
		UpdateAffiliationColor(BoundUnit->GetAffiliation());
	}
}

void ULFPHealthBarWidget::UnbindFromUnit()
{
	if (BoundUnit)
	{
		// 取消订阅事件
		BoundUnit->OnHealthChangedDelegate.RemoveDynamic(this, &ULFPHealthBarWidget::OnHealthChanged);
		BoundUnit->OnDeathDelegate.RemoveDynamic(this, &ULFPHealthBarWidget::OnUnitDeath);
		BoundUnit->OnAffiliationChangedDelegate.RemoveDynamic(this, &ULFPHealthBarWidget::OnUnitAffiliationChanged);
		if (ULFPBuffComponent* BuffComponent = BoundUnit->GetBuffComponent())
		{
			BuffComponent->OnBuffListChanged.RemoveDynamic(this, &ULFPHealthBarWidget::OnBuffListChanged);
		}
		BoundUnit = nullptr;
	}

	if (BuffContainer)
	{
		BuffContainer->ClearChildren();
	}

	HidePlannedSkillIcon();

	// 隐藏血条
	SetVisibility(ESlateVisibility::Hidden);
}

void ULFPHealthBarWidget::UpdateHealthBar(int32 CurrentHealth, int32 MaxHealth)
{
	if (!HealthProgressBar || !HealthText) return;

	// 计算血量百分比
	float HealthPercent = (MaxHealth > 0) ? (float)CurrentHealth / MaxHealth : 0.0f;

	// 更新进度条
	HealthProgressBar->SetPercent(HealthPercent);

	// 更新文本
	FString HealthString = FString::Printf(TEXT("%d / %d"), CurrentHealth, MaxHealth);
	HealthText->SetText(FText::FromString(HealthString));
}

void ULFPHealthBarWidget::ShowPlannedSkillIcon(UTexture2D* IconTexture)
{
	if (!IconTexture)
	{
		HidePlannedSkillIcon();
		return;
	}

	if (PlannedSkillIconWidget)
	{
		PlannedSkillIconWidget->SetSkillIcon(IconTexture);
		PlannedSkillIconWidget->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	if (PlannedSkillIconImage)
	{
		PlannedSkillIconImage->SetBrushFromTexture(IconTexture);
		PlannedSkillIconImage->SetVisibility(ESlateVisibility::Visible);
	}
}

void ULFPHealthBarWidget::HidePlannedSkillIcon()
{
	if (PlannedSkillIconWidget)
	{
		PlannedSkillIconWidget->SetSkillIcon(nullptr);
		PlannedSkillIconWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	if (PlannedSkillIconImage)
	{
		PlannedSkillIconImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ULFPHealthBarWidget::OnHealthChanged(int32 CurrentHealth, int32 MaxHealth)
{
	UpdateHealthBar(CurrentHealth, MaxHealth);
}

void ULFPHealthBarWidget::OnUnitDeath()
{
	// 单位死亡时隐藏血条
	SetVisibility(ESlateVisibility::Hidden);

	if (BuffContainer)
	{
		BuffContainer->ClearChildren();
	}

	HidePlannedSkillIcon();

	// 可选：显示死亡效果或文本
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(TEXT("DEAD")));
	}
}

void ULFPHealthBarWidget::OnBuffListChanged()
{
	RefreshBuffIcons();
}

void ULFPHealthBarWidget::OnUnitAffiliationChanged(ALFPTacticsUnit* Unit, EUnitAffiliation OldAffiliation, EUnitAffiliation NewAffiliation)
{
	UpdateAffiliationColor(NewAffiliation);
}

void ULFPHealthBarWidget::RefreshBuffIcons()
{
	if (!BuffContainer)
	{
		return;
	}

	BuffContainer->ClearChildren();

	if (!BoundUnit || !BuffIconWidgetClass || MaxBuffIcons <= 0)
	{
		return;
	}

	ULFPBuffComponent* BuffComponent = BoundUnit->GetBuffComponent();
	if (!BuffComponent)
	{
		return;
	}

	const TArray<FLFPBuffDisplayEntry> BuffEntries = BuffComponent->GetAggregatedVisibleBuffDisplayEntries();
	const int32 DisplayCount = FMath::Min(BuffEntries.Num(), MaxBuffIcons);
	for (int32 Index = 0; Index < DisplayCount; ++Index)
	{
		ULFPBuffIconWidget* BuffIconWidget = CreateWidget<ULFPBuffIconWidget>(this, BuffIconWidgetClass);
		if (!BuffIconWidget)
		{
			continue;
		}

		BuffIconWidget->SetBuffEntry(BuffEntries[Index]);
		BuffContainer->AddChild(BuffIconWidget);
	}
}

void ULFPHealthBarWidget::UpdateAffiliationColor(EUnitAffiliation Affiliation)
{
	//if (!BorderImage) return;

	FLinearColor HealthColor;
	switch (Affiliation)
	{
	case EUnitAffiliation::UA_Player:
		HealthColor = PlayerColor;
		break;
	case EUnitAffiliation::UA_Enemy:
		HealthColor = EnemyColor;
		break;
	case EUnitAffiliation::UA_Neutral:
		HealthColor = NeutralColor;
		break;
	default:
		HealthColor = FLinearColor::White;
	}

	//BorderImage->SetColorAndOpacity(BorderColor);
	HealthProgressBar->SetFillColorAndOpacity(HealthColor);
}
