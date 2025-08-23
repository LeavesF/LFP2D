// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPHealthBarWidget.generated.h"

class ALFPTacticsUnit;

/**
 * 
 */
UCLASS()
class LFP2D_API ULFPHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;

	// �󶨵���λ
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void BindToUnit(ALFPTacticsUnit* Unit);

	// ���λ
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UnbindFromUnit();

	// ����Ѫ����ʾ
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UpdateHealthBar(int32 CurrentHealth, int32 MaxHealth);

protected:
	// Ѫ���仯�¼�����
	UFUNCTION()
	void OnHealthChanged(int32 CurrentHealth, int32 MaxHealth);

	// ��λ�����¼�����
	UFUNCTION()
	void OnUnitDeath();

	// Ѫ���ؼ�
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UProgressBar* HealthProgressBar;

	// Ѫ���ı�
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UTextBlock* HealthText;

	// ������λ
	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	ALFPTacticsUnit* BoundUnit;

	// Ѫ����ɫ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor FullHealthColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor LowHealthColor = FLinearColor::Red;

	// ��Ѫ����ֵ (�ٷֱ�)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar", meta = (ClampMin = "0", ClampMax = "1"))
	float LowHealthThreshold = 0.3f;

	// ��Ӫ��ɫ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor PlayerColor = FLinearColor(0.1f, 1.0f, 0.1f); // ��ɫ

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor EnemyColor = FLinearColor(1.0f, 0.1f, 0.1f); // ��ɫ

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor NeutralColor = FLinearColor(0.5f, 0.5f, 0.5f); // ��ɫ

	// �߿�ͼ��
	/*UPROPERTY(BlueprintReadOnly, Category = "Health Bar", meta = (BindWidget))
	UImage* BorderImage;*/

	// ������Ӫ��ɫ
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UpdateAffiliationColor(EUnitAffiliation Affiliation);
};
