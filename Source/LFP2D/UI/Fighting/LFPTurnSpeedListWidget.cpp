// Fill out your copyright notice in the Description page of Project Settings.

#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedIconWidget.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPTurnSpeedListWidget::InitializeTurnOrder()
{
	if (!TurnManagerRef)
	{
		// ���һغϹ�����
		TArray<AActor*> FoundManagers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTurnManager::StaticClass(), FoundManagers);
		if (FoundManagers.Num() > 0)
		{
			TurnManagerRef = Cast<ALFPTurnManager>(FoundManagers[0]);
		}
	}

	if (TurnManagerRef)
	{
		// 绑定回合变化事件
		TurnManagerRef->OnTurnChanged.AddDynamic(this, &ULFPTurnSpeedListWidget::OnTurnChanged);

		// 绑定阶段变化事件
		TurnManagerRef->OnPhaseChanged.AddDynamic(this, &ULFPTurnSpeedListWidget::OnPhaseChanged);

		// 初始更新UI
		UpdateTurnOrder();
	}
}

void ULFPTurnSpeedListWidget::SetRoundNumber(int32 Round)
{
	if (RoundText)
	{
		RoundText->SetText(FText::AsNumber(Round));
	}
}
void ULFPTurnSpeedListWidget::UpdateTurnOrder()
{
	if (!TurnManagerRef)
	{
		return;
	}
	// ���ûغ��ı�
	RoundText->SetText(FText::FromString(FString::Printf(TEXT("Round %d"), TurnManagerRef->GetCurrentRound())));

	// ��������ͼ������������ؽ�
	TArray<UWidget*> ExistingIcons = UnitIconsContainer->GetAllChildren();

	TArray<ALFPTacticsUnit*> TurnOrderUnits = TurnManagerRef->GetTurnOrderUnits();
	
	for (int32 i = 0; i < TurnOrderUnits.Num(); i++)
	{
		ULFPTurnSpeedIconWidget* Icon = nullptr;

		if (i < ExistingIcons.Num())
		{
			// ��������ͼ��
			Icon = Cast<ULFPTurnSpeedIconWidget>(ExistingIcons[i]);
		}
		else
		{
			// ������ͼ��
			Icon = CreateWidget<ULFPTurnSpeedIconWidget>(this, UnitIconClass);
			UnitIconsContainer->AddChild(Icon);
		}

		// ����ͼ������
		ALFPTacticsUnit* Unit = TurnOrderUnits[i];
		if (Icon && Unit)
		{
			Icon->SetUnit(Unit);
			Icon->SetIsCurrent(TurnManagerRef->GetCurrentUnit() == Unit);
		}
	}

	// �Ƴ�����ͼ��
	for (int32 i = TurnOrderUnits.Num(); i < ExistingIcons.Num(); i++)
	{
		ExistingIcons[i]->RemoveFromParent();
	}


	//if (!UnitIconsContainer || !UnitIconClass) return;
	//// �������ͼ��
	//UnitIconsContainer->ClearChildren();
	//// Ϊÿ����λ����ͼ��
	//for (int32 i = 0; i < Units.Num(); i++)
	//{
	//	ALFPTacticsUnit* Unit = Units[i];
	//	if (Unit)
	//	{
	//		UUserWidget* IconWidget = CreateWidget<UUserWidget>(this, UnitIconClass);
	//		if (IconWidget)
	//		{
	//			// ����ͼ�꣨���������UnitIconClass����һ��Image�ؼ���Ϊ"UnitIcon"��������һ�������򹫿��������ø�����
	//			UImage* IconImage = Cast<UImage>(IconWidget->GetWidgetFromName(TEXT("UnitIcon")));
	//			if (IconImage)
	//			{
	//				// ����ͼƬ��������Ҫ��λ��ͷ����Դ�������ڵ�λ���л�ȡ
	//				// ���磺IconImage->SetBrushFromTexture(Unit->GetUnitIcon());
	//				// ��ʱ��ռλ��������Ҫ���ڵ�λ�������ӻ�ȡͷ��ĺ���
	//			}
	//			// ������ǰ��λ
	//			UWidget* Highlight = IconWidget->GetWidgetFromName(TEXT("Highlight"));
	//			if (Highlight)
	//			{
	//				Highlight->SetVisibility(i == CurrentUnitIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	//			}
	//			// ���ӵ�ˮƽ��
	//			UHorizontalBoxSlot* Slot = UnitIconsContainer->AddChildToHorizontalBox(IconWidget);
	//			Slot->SetPadding(FMargin(5, 0, 5, 0)); // ���ü��
	//			Slot->SetHorizontalAlignment(HAlign_Center);
	//			Slot->SetVerticalAlignment(VAlign_Center);
	//		}
	//	}
	//}
}

void ULFPTurnSpeedListWidget::OnTurnChanged()
{
	// 取消之前的延迟更新
	GetWorld()->GetTimerManager().ClearTimer(UpdateTimerHandle);

	// 延迟一帧更新，避免同一帧内多次更新
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ULFPTurnSpeedListWidget::UpdateTurnOrder);
}

void ULFPTurnSpeedListWidget::OnPhaseChanged(EBattlePhase NewPhase)
{
	if (!PhaseText) return;

	FString PhaseString;
	switch (NewPhase)
	{
	case EBattlePhase::BP_EnemyPlanning:
		PhaseString = TEXT("Enemy Planning");
		break;
	case EBattlePhase::BP_ActionPhase:
		PhaseString = TEXT("Action Phase");
		break;
	case EBattlePhase::BP_RoundEnd:
		PhaseString = TEXT("Round End");
		break;
	default:
		PhaseString = TEXT("");
		break;
	}

	PhaseText->SetText(FText::FromString(PhaseString));
}