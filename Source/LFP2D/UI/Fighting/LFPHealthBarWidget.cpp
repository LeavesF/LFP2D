// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/UI/Fighting/LFPHealthBarWidget.h"
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

	// 初始化数据
	UpdateHealthBar(BoundUnit->GetCurrentHealth(), BoundUnit->GetMaxHealth());

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
		BoundUnit = nullptr;
	}

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

void ULFPHealthBarWidget::OnHealthChanged(int32 CurrentHealth, int32 MaxHealth)
{
	UpdateHealthBar(CurrentHealth, MaxHealth);
}

void ULFPHealthBarWidget::OnUnitDeath()
{
	// 单位死亡时隐藏血条
	SetVisibility(ESlateVisibility::Hidden);

	// 可选：显示死亡效果或文本
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(TEXT("DEAD")));
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
