// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFPTurnManager.generated.h"

class ALFPTacticsUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnChangedSignature);

UCLASS()
class LFP2D_API ALFPTurnManager : public AActor
{
	GENERATED_BODY()
	
public:
    ALFPTurnManager();

    // ��ʼ��Ϸ�غ�ϵͳ
    void StartGame();

    // ��ʼ�»غ�
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void BeginNewRound();

    // ������ǰ�غ�
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void EndCurrentRound();

    // ��ȡ��ǰ�ж���λ
    UFUNCTION(BlueprintPure, Category = "Turn System")
    ALFPTacticsUnit* GetCurrentUnit() const { return CurrentUnit; }

    // ��ȡ��ǰ�غ���
    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetCurrentRound() const { return CurrentRound; }

    // ��ȡ��ǰ��λ�б�
    UFUNCTION(BlueprintPure, Category = "Turn System")
    TArray<ALFPTacticsUnit*> GetTurnOrderUnits() const { return TurnOrderUnits; }

    // ���ݻغϵ���һ����λ
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void PassTurn();

    // ����λ����ж�
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void OnUnitFinishedAction(ALFPTacticsUnit* Unit);

    // ע�ᵥλ���غ�ϵͳ
    void RegisterUnit(ALFPTacticsUnit* Unit);

    // �ӻغ�ϵͳע����λ
    void UnregisterUnit(ALFPTacticsUnit* Unit);

public:
    UPROPERTY(BlueprintAssignable, Category = "Turn System")
    FOnTurnChangedSignature OnTurnChanged;

protected:
    virtual void BeginPlay() override;

protected:
    // ����λ���ٶ����ȣ�
    void SortUnitsBySpeed();

    // ��ʼ��λ�غ�
    void BeginUnitTurn(ALFPTacticsUnit* Unit);

    // ������λ�غ�
    void EndUnitTurn(ALFPTacticsUnit* Unit);

    // ��λ�б�
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    TArray<ALFPTacticsUnit*> TurnOrderUnits;

    // ��ǰ�ж���λ
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    ALFPTacticsUnit* CurrentUnit;

    // ��ǰ�غ���
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    int32 CurrentRound = 0;

    // �Ƿ��ڻغ���
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    bool bIsInRound = false;
};
