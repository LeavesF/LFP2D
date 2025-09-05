// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkillComponent.generated.h"

// ί��ǩ��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillExecutedSignature, ALFPTacticsUnit*, Caster, ALFPHexTile*, TargetTile);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LFP2D_API ULFPSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULFPSkillComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ��ʼ������
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void InitializeSkills();

    // ��ȡ���п��ü���
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    TArray<ULFPSkillBase*> GetAvailableSkills() const;

    // ִ�м���
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool ExecuteSkill(ULFPSkillBase* Skill, ALFPHexTile* TargetTile);

    // ��ȡĬ�Ϲ�������
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    ULFPSkillBase* GetDefaultAttackSkill() const;

    // �غϿ�ʼ����
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void OnTurnStarted();

    // ����ִ���¼�
    UPROPERTY(BlueprintAssignable, Category = "Skill")
    FOnSkillExecutedSignature OnSkillExecuted;

private:
    // ���м���
    UPROPERTY(VisibleAnywhere, Category = "Skill")
    TArray<ULFPSkillBase*> Skills;

    // Ĭ�Ϲ�������
    UPROPERTY()
    ULFPSkillBase* DefaultAttackSkill;

    // ���������ʲ�
    UPROPERTY(EditAnywhere, Category = "Skill")
    class ULFPSkillDataAsset* SkillData;
};
