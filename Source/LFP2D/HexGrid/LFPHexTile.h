// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h"
#include "LFPHexTile.generated.h"

class UPaperSpriteComponent;
class UPaperSprite;
class ALFPTacticsUnit;

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
	FVector ToWorldLocation(float Size, float VerticalScale = 1.0f) const
	{
		// 平顶六边形布局
		const float HorizontalSpacing = Size * FMath::Sqrt(3.f); // 水平间距
		const float VerticalSpacing = Size * 1.5f * VerticalScale; // 垂直间距应用缩放

		float X = HorizontalSpacing * (Q + R * 0.5f);
		float Y = VerticalSpacing * R;

		return FVector(X, Y, 0);
	}

	// 距离计算
	static int32 Distance(const FLFPHexCoordinates& A, const FLFPHexCoordinates& B)
	{
		return (FMath::Abs(A.Q - B.Q) +
			FMath::Abs(A.R - B.R) +
			FMath::Abs(A.S - B.S)) / 2;
	}

	// 获取所有相邻坐标
	TArray<FLFPHexCoordinates> GetNeighbors() const
	{
		// 尖顶六边形的邻居方向
		static const TArray<FLFPHexCoordinates> Directions = {
			FLFPHexCoordinates(+1,  0), // 右
			FLFPHexCoordinates(+1, -1), // 右上
			FLFPHexCoordinates(0, -1), // 左上
			FLFPHexCoordinates(-1,  0), // 左
			FLFPHexCoordinates(-1, +1), // 左下
			FLFPHexCoordinates(0, +1)  // 右下
		};

		TArray<FLFPHexCoordinates> Neighbors;
		for (const auto& Dir : Directions)
		{
			Neighbors.Add(FLFPHexCoordinates(Q + Dir.Q, R + Dir.R));
		}
		return Neighbors;
	}

	static FLFPHexCoordinates FromWorldLocation(const FVector2D& WorldLocation, float HexSize, float VerticalScale)
	{
		// 平顶六边形坐标转换公式
		const float Sqrt3 = FMath::Sqrt(3.f);

		float unit_x = HexSize * Sqrt3;
		float unit_y = HexSize * 1.5f;
		float minDistanceLimit = HexSize * Sqrt3 * 0.5f;
		float WorldLocation_X = WorldLocation.X;
		float WorldLocation_Y = WorldLocation.Y / VerticalScale;

		TArray<FVector2D> anchors;
		anchors.Add(FVector2D::ZeroVector);
		anchors.Add(FVector2D::ZeroVector);
		anchors.Add(FVector2D::ZeroVector);
		float distance;
		float minDistance = 10000;
		int index = 0;

		int32 index_x = WorldLocation_X / unit_x - (WorldLocation_X < 0 ? 1 : 0);
		int32 index_y = WorldLocation_Y / unit_y - (WorldLocation_Y < 0 ? 1 : 0);
		anchors[0].X = index_x * unit_x;
		anchors[1].X = (index_x + 0.5) * unit_x;
		anchors[2].X = (index_x + 1) * unit_x;
		if (index_y % 2 == 0)
		{
			anchors[0].Y = anchors[2].Y = index_y * unit_y;
			anchors[1].Y = (index_y + 1) * unit_y;
		}
		else
		{
			anchors[0].Y = anchors[2].Y = (index_y + 1) * unit_y;
			anchors[1].Y = index_y * unit_y;
		}
		// 找出距离坐标最近的一个点
		for (int i = 0; i < 3; i++)
		{
			//求出距离的平方
			UE_LOG(LogTemp, Warning, TEXT("anchor:(%f,%f)"), anchors[i].X, anchors[i].Y);
			distance = FVector2D::Distance(FVector2D(WorldLocation_X, WorldLocation_Y), FVector2D(anchors[i].X, anchors[i].Y));
			UE_LOG(LogTemp, Warning, TEXT("distance:(%f)"), distance);
			//如果已经肯定被捕获
			if (distance < minDistanceLimit)
			{
				index = i;
				break;
			}

			//更新最小距离值和索引
			if (distance < minDistance)
			{
				minDistance = distance;
				index = i;
			}
		}

		int32 q = FMath::RoundToFloat((anchors[index].X / Sqrt3 - anchors[index].Y / 3.f) / HexSize);
		int32 r = FMath::RoundToFloat(anchors[index].Y * 2.f / 3.f / HexSize);
		
		return FLFPHexCoordinates(q, r);
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
	FLFPHexCoordinates GetCoordinates() { return Coordinates; }
	// 设置坐标数据
	void SetCoordinates(const FLFPHexCoordinates& NewCoords) { Coordinates = NewCoords; }

	bool IsWalkable() { return bIsWalkable; }
	bool IsOccupied() { return bIsOccupied; }

	void SetIsWalkable(bool bInIsWalkable) { bIsWalkable = bInIsWalkable; }
	void SetIsOccupied(bool bInIsOccupied) { bIsOccupied = bInIsOccupied; }

	void SetUnitOnTile(ALFPTacticsUnit* Unit) { CurrentUnit = Unit; }
	// 设置格子状态
	void SetState(bool bWalkable, bool bOccupied);

	// 设置格子状态
	//void SetSelected(bool bSelect);

	// 高亮显示
	void Highlight(bool bActive);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

protected:
	FLFPHexCoordinates Coordinates;

	bool bIsWalkable = true;
	bool bIsOccupied = false;
	//bool bIsSelect = false;

	TObjectPtr<ALFPTacticsUnit> CurrentUnit;

public:

	// 添加路径高亮功能
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetPathHighlight(bool bActive);

	// 添加移动范围高亮功能
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetMovementHighlight(bool bActive);

	// 添加攻击范围高亮功能
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetAttackHighlight(bool bActive);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPaperSprite> DefaultSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Sprites")
	TObjectPtr<UPaperSprite> PathSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Sprites")
	TObjectPtr<UPaperSprite> MovementRangeSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Sprites")
	TObjectPtr<UPaperSprite> AttackRangeSprite;
};
