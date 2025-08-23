// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/UI/Fighting/LFPHealthBarWidget.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ȷ���ؼ��Ѱ�
	if (HealthProgressBar && HealthText)
	{
		// ��ʼ����Ѫ��
		//SetVisibility(ESlateVisibility::Hidden);
	}
}

void ULFPHealthBarWidget::BindToUnit(ALFPTacticsUnit* Unit)
{
	if (!Unit || BoundUnit == Unit) return;

	// ������е�λ
	UnbindFromUnit();

	// ���µ�λ
	BoundUnit = Unit;

	// ���¼�
	BoundUnit->OnHealthChangedDelegate.AddDynamic(this, &ULFPHealthBarWidget::OnHealthChanged);
	BoundUnit->OnDeathDelegate.AddDynamic(this, &ULFPHealthBarWidget::OnUnitDeath);

	// ��ʼ����
	UpdateHealthBar(BoundUnit->GetCurrentHealth(), BoundUnit->GetMaxHealth());

	// ��ʾѪ��
	SetVisibility(ESlateVisibility::Visible);

	// ������Ӫ��ɫ
	if (BoundUnit)
	{
		UpdateAffiliationColor(BoundUnit->GetAffiliation());
	}
}

void ULFPHealthBarWidget::UnbindFromUnit()
{
	if (BoundUnit)
	{
		// ����¼�
		BoundUnit->OnHealthChangedDelegate.RemoveDynamic(this, &ULFPHealthBarWidget::OnHealthChanged);
		BoundUnit->OnDeathDelegate.RemoveDynamic(this, &ULFPHealthBarWidget::OnUnitDeath);
		BoundUnit = nullptr;
	}

	// ����Ѫ��
	SetVisibility(ESlateVisibility::Hidden);
}

void ULFPHealthBarWidget::UpdateHealthBar(int32 CurrentHealth, int32 MaxHealth)
{
	if (!HealthProgressBar || !HealthText) return;

	// ����Ѫ���ٷֱ�
	float HealthPercent = (MaxHealth > 0) ? (float)CurrentHealth / MaxHealth : 0.0f;

	// ���½�����
	HealthProgressBar->SetPercent(HealthPercent);

	// �����ı�
	FString HealthString = FString::Printf(TEXT("%d / %d"), CurrentHealth, MaxHealth);
	HealthText->SetText(FText::FromString(HealthString));
}

void ULFPHealthBarWidget::OnHealthChanged(int32 CurrentHealth, int32 MaxHealth)
{
	UpdateHealthBar(CurrentHealth, MaxHealth);
}

void ULFPHealthBarWidget::OnUnitDeath()
{
	// ��λ����ʱ����Ѫ��
	SetVisibility(ESlateVisibility::Hidden);

	// ��ѡ����ʾ����Ч�����ı�
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