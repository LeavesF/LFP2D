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

	int32 Q; // �������� q
	int32 R; // �������� r
	int32 S; // �������� s������ó���

	FLFPHexCoordinates() : Q(0), R(0), S(0) {}

	FLFPHexCoordinates(int32 q, int32 r) : Q(q), R(r), S(-q - r) {}

	// ����ת������
	FVector ToWorldLocation(float Size, float VerticalScale = 1.0f) const
	{
		// ƽ�������β���
		const float HorizontalSpacing = Size * FMath::Sqrt(3.f); // ˮƽ���
		const float VerticalSpacing = Size * 1.5f * VerticalScale; // ��ֱ���Ӧ������

		float X = HorizontalSpacing * (Q + R * 0.5f);
		float Y = VerticalSpacing * R;

		return FVector(X, Y, 0);
	}

	// �������
	static int32 Distance(const FLFPHexCoordinates& A, const FLFPHexCoordinates& B)
	{
		return (FMath::Abs(A.Q - B.Q) +
			FMath::Abs(A.R - B.R) +
			FMath::Abs(A.S - B.S)) / 2;
	}

	// ��ȡ������������
	TArray<FLFPHexCoordinates> GetNeighbors() const
	{
		// �ⶥ�����ε��ھӷ���
		static const TArray<FLFPHexCoordinates> Directions = {
			FLFPHexCoordinates(+1,  0), // ��
			FLFPHexCoordinates(+1, -1), // ����
			FLFPHexCoordinates(0, -1), // ����
			FLFPHexCoordinates(-1,  0), // ��
			FLFPHexCoordinates(-1, +1), // ����
			FLFPHexCoordinates(0, +1)  // ����
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
		// ƽ������������ת����ʽ
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
		// �ҳ��������������һ����
		for (int i = 0; i < 3; i++)
		{
			//��������ƽ��
			UE_LOG(LogTemp, Warning, TEXT("anchor:(%f,%f)"), anchors[i].X, anchors[i].Y);
			distance = FVector2D::Distance(FVector2D(WorldLocation_X, WorldLocation_Y), FVector2D(anchors[i].X, anchors[i].Y));
			UE_LOG(LogTemp, Warning, TEXT("distance:(%f)"), distance);
			//����Ѿ��϶�������
			if (distance < minDistanceLimit)
			{
				index = i;
				break;
			}

			//������С����ֵ������
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
	// ������������
	void SetCoordinates(const FLFPHexCoordinates& NewCoords) { Coordinates = NewCoords; }

	bool IsWalkable() { return bIsWalkable; }
	bool IsOccupied() { return bIsOccupied; }

	void SetIsWalkable(bool bInIsWalkable) { bIsWalkable = bInIsWalkable; }
	void SetIsOccupied(bool bInIsOccupied) { bIsOccupied = bInIsOccupied; }

	void SetUnitOnTile(ALFPTacticsUnit* Unit) { CurrentUnit = Unit; }
	// ���ø���״̬
	void SetState(bool bWalkable, bool bOccupied);

	// ���ø���״̬
	//void SetSelected(bool bSelect);

	// ������ʾ
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

	// ���·����������
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetPathHighlight(bool bActive);

	// ����ƶ���Χ��������
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetMovementHighlight(bool bActive);

	// ��ӹ�����Χ��������
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
