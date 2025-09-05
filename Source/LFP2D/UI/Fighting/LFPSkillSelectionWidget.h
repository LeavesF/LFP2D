// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "GameplayTagContainer.h"
#include "LFPSkillSelectionWidget.generated.h"

class ULFPSkillBase;
class ALFPTacticsUnit;
class ULFPSkillButtonWidget;
/**
 * 
 */
 // �������򷽷�ö��
UENUM(BlueprintType)
enum class ESkillSortMethod : uint8
{
    ByName,     // ����������
    ByCooldown, // ����ȴʱ������
    ByCost,     // ����������
    ByType      // ����������
};

// ����ѡ��ί��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillChosenSignature, ULFPSkillBase*, SelectedSkill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSkillConfirmedSignature, ULFPSkillBase*, SelectedSkill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSelectionCanceledSignature);

UCLASS()
class LFP2D_API ULFPSkillSelectionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // ���캯��
    ULFPSkillSelectionWidget(const FObjectInitializer& ObjectInitializer);

    // ��ʼ�������б�
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void InitializeSkills(ALFPTacticsUnit* Unit);

    // ��ʾ����ѡ�񴰿�
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void Show();

    // ���ؼ���ѡ�񴰿�
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void Hide();

    // ���ü��ܹ�����
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void SetSkillFilter(const FGameplayTagContainer& FilterTags);

    // ������ܹ�����
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void ClearSkillFilter();

    // ������
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void SortSkills(ESkillSortMethod SortMethod);

    // ˢ�¼��ܰ�ť״̬
    UFUNCTION(BlueprintCallable, Category = "Skill Selection")
    void RefreshSkillButtons();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    // ����ѡ���¼�����
    UFUNCTION()
    void OnSkillSelected(ULFPSkillBase* Skill);

    // ������ͣ�¼�����
    UFUNCTION()
    void OnSkillHovered(ULFPSkillBase* Skill);

    // ����ȡ����ͣ�¼�����
    UFUNCTION()
    void OnSkillUnhovered();

    // ȷ�ϰ�ť����¼�
    UFUNCTION()
    void OnConfirmClicked();

    // ȡ����ť����¼�
    UFUNCTION()
    void OnCancelClicked();

    // ���¼���������ʾ
    UFUNCTION()
    void UpdateSkillDetails(ULFPSkillBase* Skill);

    // ���µ�λ��Ϣ��ʾ
    UFUNCTION()
    void UpdateUnitInfo();

    // ����ȷ�ϰ�ť״̬
    UFUNCTION()
    void UpdateConfirmButtonState();

    // ��ȡ���˺�ļ����б�
    UFUNCTION()
    TArray<ULFPSkillBase*> GetFilteredSkills(ALFPTacticsUnit* Unit) const;

public:
    // ί�У����ܱ�ѡ�У���δȷ�ϣ�
    UPROPERTY(BlueprintAssignable, Category = "Skill Selection")
    FSkillChosenSignature OnSkillChosen;

    // ί�У����ܱ�ȷ��ѡ��
    UPROPERTY(BlueprintAssignable, Category = "Skill Selection")
    FSkillConfirmedSignature OnSkillConfirmed;

    // ί�У�ѡ��ȡ��
    UPROPERTY(BlueprintAssignable, Category = "Skill Selection")
    FSelectionCanceledSignature OnSelectionCanceled;

protected:
    // ������������
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UUniformGridPanel* SkillGrid;

    // �����������
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UWidget* SkillDetailsPanel;

    // ���������ı�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillNameText;

    // ���������ı�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillDescriptionText;

    // ���ܷ�Χ�ı�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillRangeText;

    // ������ȴ�ı�
    UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    UTextBlock* SkillCooldownText;

    //// ����ͼ��
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UImage* SkillIcon;

    //// ��λ�����ı�
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* UnitNameText;

    //// �ж����ı�
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* ActionPointsText;

    //// Ѫ���ı�
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* HealthText;

    //// ȷ�ϰ�ť
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UButton* ConfirmButton;

    //// ȡ����ť
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UButton* CancelButton;

    //// ȷ����ʾ�ı�
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* ConfirmHintText;

    //// �޼�����ʾ�ı�
    //UPROPERTY(BlueprintReadOnly, Category = "Skill Selection", meta = (BindWidget))
    //UTextBlock* NoSkillsText;

    // ���ܰ�ť��
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    TSubclassOf<ULFPSkillButtonWidget> SkillButtonClass;

    //// ��ͣ��Ч
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* HoverSound;

    //// ѡ����Ч
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* SelectSound;

    //// ȷ����Ч
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* ConfirmSound;

    //// ȡ����Ч
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Selection")
    //USoundBase* CancelSound;

private:
    // ��ǰѡ�еļ���
    UPROPERTY()
    ULFPSkillBase* SelectedSkill;

    // ������λ
    UPROPERTY()
    ALFPTacticsUnit* OwnerUnit;

    // ���м��ܰ�ť
    UPROPERTY()
    TArray<ULFPSkillButtonWidget*> SkillButtons;

    // ���ܹ��˱�ǩ
    UPROPERTY()
    FGameplayTagContainer SkillFilterTags;

    // �������
    UPROPERTY(EditAnywhere, Category = "Skill Selection")
    int32 MaxColumns;
};
