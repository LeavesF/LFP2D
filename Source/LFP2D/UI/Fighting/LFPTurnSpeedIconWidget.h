// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPTurnSpeedIconWidget.generated.h"

class ALFPTacticsUnit;
/**
 * 
 */
UCLASS()
class LFP2D_API ULFPTurnSpeedIconWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Unit Icon")
    void SetUnit(ALFPTacticsUnit* Unit);

    UFUNCTION(BlueprintCallable, Category = "Unit Icon")
    void SetIsCurrent(bool bCurrent);

    //virtual void NativeConstruct() override;
    //virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Icon")
    void UpdateAppearance();

    UPROPERTY(BlueprintReadOnly, Category = "Unit Icon", meta = (BindWidget))
    class UImage* IconImage;

    UPROPERTY(BlueprintReadOnly, Category = "Unit Icon", meta = (BindWidget))
    class UImage* HighlightBorder;

    UPROPERTY(BlueprintReadOnly, Category = "Unit Icon", meta = (BindWidget))
    class UImage* StatusIcon;

    UPROPERTY(BlueprintReadOnly, Category = "Unit Icon")
    ALFPTacticsUnit* UnitRef;

    UPROPERTY(BlueprintReadOnly, Category = "Unit Icon")
    bool bIsCurrent;
};
