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

    // 设置/获取当前坐标
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    FLFPHexCoordinates GetCurrentCoordinates() const { return CurrentCoordinates; }

    // 移动到目标格子
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void MoveToTile(ALFPHexTile* TargetTile);

    // 设置选中状态
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetSelected(bool bSelected);

    // 获取移动范围
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetMovementRange() const { return MovementRange; }

    // 消耗行动点
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void ConsumeActionPoints(int32 Amount);

    // 检查是否有足够行动点
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughActionPoints(int32 Required) const;

    ALFPHexGridManager* GetGridManager() const;
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 移动动画更新
    UFUNCTION()
    void UpdateMoveAnimation(float Value);

    // 完成移动
    void FinishMove();

private:
    // 单位属性
    UPROPERTY(EditAnywhere, Category = "Unit Stats")
    int32 MovementRange = 5;

    UPROPERTY(EditAnywhere, Category = "Unit Stats")
    int32 MaxActionPoints = 3;

    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    int32 CurrentActionPoints;

    // 当前坐标
    FLFPHexCoordinates CurrentCoordinates;

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

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UArrowComponent* FacingDirection;
};
