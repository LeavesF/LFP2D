#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "LFPWorldMapPawn.generated.h"

class UPaperSpriteComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldMapPawnMoveComplete);

/**
 * 世界地图玩家棋子：在节点间移动时播放平滑动画
 */
UCLASS()
class LFP2D_API ALFPWorldMapPawn : public AActor
{
	GENERATED_BODY()

public:
	ALFPWorldMapPawn();

	virtual void Tick(float DeltaTime) override;

	// 移动到目标位置（启动动画）
	UFUNCTION(BlueprintCallable, Category = "World Map|Pawn")
	void MoveToLocation(FVector TargetLocation);

	// 是否正在移动中
	UFUNCTION(BlueprintPure, Category = "World Map|Pawn")
	bool IsMoving() const { return bIsMoving; }

	// 直接设置位置（无动画，用于初始化/恢复）
	UFUNCTION(BlueprintCallable, Category = "World Map|Pawn")
	void SetLocationImmediate(FVector Location);

	// 移动完成委托
	UPROPERTY(BlueprintAssignable)
	FOnWorldMapPawnMoveComplete OnMoveComplete;

protected:
	// 棋子精灵
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> PawnSpriteComponent;

	// 移动动画曲线（蓝图中配置，0→1）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Map|Pawn")
	TObjectPtr<UCurveFloat> MoveCurve;

	// 移动动画时长（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Map|Pawn")
	float MoveDuration = 0.5f;

private:
	// Timeline 回调
	UFUNCTION()
	void UpdateMoveAnimation(float Alpha);

	UFUNCTION()
	void OnMoveFinished();

	FTimeline MoveTimeline;
	FVector MoveStartLocation;
	FVector MoveTargetLocation;
	bool bIsMoving = false;
};
