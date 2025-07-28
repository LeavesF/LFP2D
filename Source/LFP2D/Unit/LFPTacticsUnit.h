// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "Components/TimelineComponent.h"
#include "PaperSpriteComponent.h"
#include "Components/ArrowComponent.h"
#include "LFPTacticsUnit.generated.h"

class ALFPHexGridManager;

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

    // �ƶ���Ŀ�����
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void MoveToTile(ALFPHexTile* TargetTile);

    // ����ѡ��״̬
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetSelected(bool bSelected);

    // ��ȡ�ƶ���Χ
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetMovementRange() const { return MovementRange; }

    // �����ж���
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void ConsumeActionPoints(int32 Amount);

    // ����Ƿ����㹻�ж���
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughActionPoints(int32 Required) const;

    ALFPHexGridManager* GetGridManager() const;
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // �ƶ���������
    UFUNCTION()
    void UpdateMoveAnimation(float Value);

    // ����ƶ�
    void FinishMove();

private:
    // ��λ����
    UPROPERTY(EditAnywhere, Category = "Unit Stats")
    int32 MovementRange = 5;

    UPROPERTY(EditAnywhere, Category = "Unit Stats")
    int32 MaxActionPoints = 3;

    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    int32 CurrentActionPoints;

    // ��ǰ����
    FLFPHexCoordinates CurrentCoordinates;

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

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UArrowComponent* FacingDirection;
};
