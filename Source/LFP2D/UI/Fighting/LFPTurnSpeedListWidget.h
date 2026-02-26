// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPTurnSpeedListWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UImage;

/**
 * 
 */
UCLASS()
class LFP2D_API ULFPTurnSpeedListWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void InitializeTurnOrder();

	// ���ûغ���
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void SetRoundNumber(int32 Round);

	// �����ж���λ�б�
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void UpdateTurnOrder();

	UFUNCTION()
	void OnTurnChanged();

	// 阶段变化响应
	UFUNCTION()
	void OnPhaseChanged(EBattlePhase NewPhase);

protected:
	// 显示回合数的文本
	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoundText;

	// 阶段文本（可选绑定）
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* PhaseText;
	// ���ڷ��õ�λͷ���ˮƽ��
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* UnitIconsContainer;
	// ��λͷ��Ŀؼ���ͼ�ࣨ����ͼ�����ã�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn UI")
	TSubclassOf<UUserWidget> UnitIconClass;

	UPROPERTY(BlueprintReadOnly, Category = "Turn Order")
	ALFPTurnManager* TurnManagerRef;

	FTimerHandle UpdateTimerHandle;
};
