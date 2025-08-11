// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	// ���ûغ���
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void SetRoundNumber(int32 Round);
	// �����ж���λ�б�
	UFUNCTION(BlueprintCallable, Category = "Turn UI")
	void UpdateTurnOrder(const TArray<class ALFPTacticsUnit*>& Units, int32 CurrentUnitIndex);
protected:
	// ������ʾ�غ������ı�
	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoundText;
	// ���ڷ��õ�λͷ���ˮƽ��
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* UnitIconsContainer;
	// ��λͷ��Ŀؼ���ͼ�ࣨ����ͼ�����ã�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn UI")
	TSubclassOf<UUserWidget> UnitIconClass;
};
