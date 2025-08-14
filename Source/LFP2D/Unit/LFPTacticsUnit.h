// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "Components/TimelineComponent.h"
#include "PaperSpriteComponent.h"
#include "LFPTacticsUnit.generated.h"

class ALFPHexGridManager;
class ALFPTurnManager;

UCLASS()
class LFP2D_API ALFPTacticsUnit : public AActor
{
	GENERATED_BODY()
	
public:
    ALFPTacticsUnit();

    // ����/��ȡ��ǰ����
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    FLFPHexCoordinates GetCurrentCoordinates() const { return CurrentCoordinates; }

    //// ��ȡ���ƶ���Χ
    //UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    //TArray<ALFPHexTile*> GetMovementRangeTiles();

    // ���ƶ���Χ����
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    TArray<ALFPHexTile*> MovementRangeTiles;

    // �ƶ���Ŀ�����
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void MoveToTile(ALFPHexTile* TargetTile);

    // ����ѡ��״̬
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetSelected(bool bSelected);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool IsSelected() const { return bIsSelected; }

    // ��ȡ�ƶ���Χ
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetMovementRange() const { return MovementRange; }

    // �����ж���
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void ConsumeMovePoints(int32 Amount);

    // ����Ƿ����㹻�ж���
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughMovePoints(int32 Required) const;

    ALFPHexGridManager* GetGridManager() const;
    ALFPTurnManager* GetTurnManager() const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // �ƶ���������
    UFUNCTION()
    void UpdateMoveAnimation(float Value);

    // ����ƶ�
    void FinishMove();

    // �������
    void SnapToGrid();

protected:
    // ��λ����
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MovementRange = 5;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MaxMovePoints = 3;

    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    int32 CurrentMovePoints;

    // ��ǰ����
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_Q = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_R = 0;

    FLFPHexCoordinates CurrentCoordinates;

    // ѡ��״̬
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    bool bIsSelected = false;

    // �ƶ�״̬
    UPROPERTY(Transient)
    ALFPHexTile* TargetTile;

    TArray<ALFPHexTile*> MovePath;
    int32 CurrentPathIndex;
    float MoveProgress;

    // ����
    FTimerHandle MoveTimerHandle;
    FTimeline MoveTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UCurveFloat* MoveCurve;

    // ���
    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UPaperSpriteComponent* SpriteComponent;

public:
    // �غ�ϵͳ����
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void ResetForNewRound();

    UFUNCTION(BlueprintPure, Category = "Turn System")
    bool CanAct() const { return !bHasActed && bOnTurn; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetSpeed() const { return Speed; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    bool HasActed() const { return bHasActed; }

    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void SetHasActed(bool bActed) { bHasActed = bActed; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn System")
    UTexture2D* GetUnitIcon() const { return UnitIconTexture; }

    // �غ��¼�
    UFUNCTION()
    void OnTurnStarted();

    UFUNCTION()
    void OnTurnEnded();

public:
    // �غ�ϵͳ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Speed = 5; // �ٶ�ֵ�������ж�˳��

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bHasActed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bOnTurn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn System")
    UTexture2D* UnitIconTexture;

public:
    UFUNCTION(BlueprintCallable, Category = "Mouse Input")
    void OnMouseEnter();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mouse Input")
    void OnMouseExit();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mouse Input")
    void OnMouseClick();
};
