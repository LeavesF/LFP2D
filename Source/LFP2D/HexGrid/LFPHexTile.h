// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h"
#include "LFPHexTile.generated.h"

class UPaperSpriteComponent;
class UPaperSprite;

USTRUCT(BlueprintType)
struct FLFPHexCoordinates
{
	GENERATED_USTRUCT_BODY()

	int32 Q; // 轴向坐标 q
	int32 R; // 轴向坐标 r
	int32 S; // 立方坐标 s（计算得出）

	FLFPHexCoordinates() : Q(0), R(0), S(0) {}

	FLFPHexCoordinates(int32 q, int32 r) : Q(q), R(r), S(-q - r) {}

	// 坐标转换方法
	FVector ToWorldLocation(float HexSize) const
	{
		const float X = HexSize * (FMath::Sqrt(3.f) * Q + FMath::Sqrt(3.f) / 2 * R);
		const float Y = HexSize * (3.0f / 2 * R);
		return FVector(X, Y, 0);
	}

	// 距离计算
	static int32 Distance(const FLFPHexCoordinates& A, const FLFPHexCoordinates& B)
	{
		return (FMath::Abs(A.Q - B.Q) +
			FMath::Abs(A.R - B.R) +
			FMath::Abs(A.S - B.S)) / 2;
	}
};

UCLASS()
class LFP2D_API ALFPHexTile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALFPHexTile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	// 设置坐标数据
	void SetCoordinates(const FLFPHexCoordinates& NewCoords) { Coordinates = NewCoords; }

	// 设置格子状态
	void SetState(bool bWalkable, bool bOccupied);

	// 高亮显示
	void Highlight(bool bActive);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr< UPaperSpriteComponent> SpriteComponent;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPaperSprite> DefaultSprite;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPaperSprite> HighlightedSprite;

private:
	FLFPHexCoordinates Coordinates;
	bool bIsWalkable = true;
	bool bIsOccupied = false;
};
