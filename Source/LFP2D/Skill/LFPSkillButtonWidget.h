// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "LFPSkillButtonWidget.generated.h"

// ǰ������
class ULFPSkillBase;

// ���ܰ�ťί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillButtonClickedSignature, ULFPSkillBase*, Skill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillButtonHoveredSignature, ULFPSkillBase*, Skill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSkillButtonUnhoveredSignature);
/**
 * 
 */
UCLASS()
class LFP2D_API ULFPSkillButtonWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // ���캯��
    ULFPSkillButtonWidget(const FObjectInitializer& ObjectInitializer);

    // ��ʼ�����ܰ�ť
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void Initialize(ULFPSkillBase* Skill);

    // ˢ�°�ť״̬
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void RefreshState();

    // ��ȡ�����ļ���
    UFUNCTION(BlueprintPure, Category = "Skill Button")
    ULFPSkillBase* GetSkill() const { return AssociatedSkill; }

    // ���ð�ť�Ƿ����
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void SetButtonEnabled(bool bEnabled);

    // ���ð�ťѡ��״̬
    UFUNCTION(BlueprintCallable, Category = "Skill Button")
    void SetSelected(bool bSelected);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
    // ��ť����¼�����
    UFUNCTION()
    void OnButtonClicked();

    // ���°�ť���
    UFUNCTION()
    void UpdateAppearance();

    // ������ȴ��ʾ
    UFUNCTION()
    void UpdateCooldownDisplay();

public:
    // ί�У���ť�����
    UPROPERTY(BlueprintAssignable, Category = "Skill Button")
    FSkillButtonClickedSignature OnButtonClickedDelegate;

    // ί�У���ť����ͣ
    UPROPERTY(BlueprintAssignable, Category = "Skill Button")
    FSkillButtonHoveredSignature OnButtonHoveredDelegate;

    // ί�У���ťȡ����ͣ
    UPROPERTY(BlueprintAssignable, Category = "Skill Button")
    FSkillButtonUnhoveredSignature OnButtonUnhoveredDelegate;

protected:
    // ��ť���
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UButton* SkillButton;

    // ���������ı�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UTextBlock* SkillNameText;

    // ����ͼ��
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UImage* SkillIcon;

    // ��ȴ�ı�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UTextBlock* CooldownText;

    // �����ı�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UTextBlock* CostText;

    // ѡ�и����߿�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UImage* SelectionBorder;

    // ����������
    UPROPERTY(BlueprintReadOnly, Category = "Skill Button", meta = (BindWidget))
    UImage* DisabledOverlay;

    // Ĭ�ϰ�ť��ʽ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FButtonStyle DefaultButtonStyle;

    // ѡ�а�ť��ʽ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FButtonStyle SelectedButtonStyle;

    // �����ð�ť��ʽ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FButtonStyle DisabledButtonStyle;

    // ��ȴ���ı���ɫ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FSlateColor CooldownTextColor;

    // �����ı���ɫ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    FSlateColor AvailableTextColor;

    // �����Ч
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    USoundBase* ClickSound;

    // ��ͣ��Ч
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Button")
    USoundBase* HoverSound;

private:
    // �����ļ���
    UPROPERTY()
    ULFPSkillBase* AssociatedSkill;

    // �Ƿ�ѡ��
    UPROPERTY()
    bool bIsSelected;
};
