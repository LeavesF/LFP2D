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

    // 设置/获取当前坐标
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    FLFPHexCoordinates GetCurrentCoordinates() const { return CurrentCoordinates; }

    //// 获取可移动范围
    //UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    //TArray<ALFPHexTile*> GetMovementRangeTiles();

    // 可移动范围格子
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    TArray<ALFPHexTile*> MovementRangeTiles;

    // 移动到目标格子
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void MoveToTile(ALFPHexTile* TargetTile);

    // 设置选中状态
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetSelected(bool bSelected);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool IsSelected() const { return bIsSelected; }

    // 获取移动范围
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetMovementRange() const { return MovementRange; }

    // 消耗行动点
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void ConsumeMovePoints(int32 Amount);

    // 检查是否有足够行动点
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughMovePoints(int32 Required) const;

    ALFPHexGridManager* GetGridManager() const;
    ALFPTurnManager* GetTurnManager() const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // 移动动画更新
    UFUNCTION()
    void UpdateMoveAnimation(float Value);

    // 完成移动
    void FinishMove();

    // 对齐格子
    void SnapToGrid();

protected:
    // 单位属性
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MovementRange = 5;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MaxMovePoints = 3;

    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    int32 CurrentMovePoints;

    // 当前坐标
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_Q = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_R = 0;

    FLFPHexCoordinates CurrentCoordinates;

    // 选中状态
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    bool bIsSelected = false;

    // 移动状态
    UPROPERTY(Transient)
    ALFPHexTile* TargetTile;

    TArray<ALFPHexTile*> MovePath;
    int32 CurrentPathIndex;
    float MoveProgress;

    // 动画
    FTimerHandle MoveTimerHandle;
    FTimeline MoveTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UCurveFloat* MoveCurve;

    // 组件
    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UPaperSpriteComponent* SpriteComponent;

public:
    // 回合系统函数
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

    // 回合事件
    UFUNCTION()
    void OnTurnStarted();

    UFUNCTION()
    void OnTurnEnded();

public:
    // 回合系统属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Speed = 5; // 速度值（决定行动顺序）

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
