// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPAIController.generated.h"

class ALFPTacticsUnit;
class ALFPHexGridManager;
class ALFPHexTile;
class ULFPEnemyBehaviorData;
/**
 * 
 */
UCLASS()
class LFP2D_API ALFPAIController : public AAIController
{
	GENERATED_BODY()

public:
    ALFPAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    // ��ʼ��λ�غ�
    UFUNCTION()
    void StartUnitTurn();

    // ������λ�غ�
    UFUNCTION()
    void EndUnitTurn();

    // ��ȡ��Ϊ��
    UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

    // ��ȡ���������
    ALFPHexGridManager* GetGridManager() const { return GridManager; }

    // Ѱ�����Ŀ��
    UFUNCTION(BlueprintCallable, Category = "AI")
    ALFPTacticsUnit* FindBestTarget() const;

    // 根据技能的仇恨值公式，从所有玩家单位中找到最优攻击目标
    UFUNCTION(BlueprintCallable, Category = "AI")
    ALFPTacticsUnit* FindBestSkillTarget(ULFPSkillBase* Skill) const;

    // Ѱ������ƶ�λ��
    UFUNCTION(BlueprintCallable, Category = "AI")
    ALFPHexTile* FindBestMovementTile(ALFPTacticsUnit* Target) const;

    // ������вֵ
    UFUNCTION(BlueprintCallable, Category = "AI")
    float CalculateThreatValue(ALFPTacticsUnit* Target) const;

    // ����λ�ü�ֵ
    UFUNCTION(BlueprintCallable, Category = "AI")
    float CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target) const;

protected:
    // ��Ϊ���ʲ�
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

    // �ڰ����
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    UBlackboardComponent* BlackboardComponent;

public:
    ALFPTacticsUnit* GetControlledUnit();

    void SetControlledUnit(ALFPTacticsUnit* NewUnit);

protected:
    // ��ǰ���Ƶĵ�λ
    UPROPERTY()
    ALFPTacticsUnit* ControlledUnit;

    // ���������
    UPROPERTY()
    ALFPHexGridManager* GridManager;

    // AI ��Ϊ����
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    ULFPEnemyBehaviorData* BehaviorData;
};
