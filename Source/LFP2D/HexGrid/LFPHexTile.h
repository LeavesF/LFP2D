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

	int32 Q; // �������� q
	int32 R; // �������� r
	int32 S; // �������� s������ó���

	FLFPHexCoordinates() : Q(0), R(0), S(0) {}

	FLFPHexCoordinates(int32 q, int32 r) : Q(q), R(r), S(-q - r) {}

	// ����ת������
	FVector ToWorldLocation(float HexSize) const
	{
		const float X = HexSize * (FMath::Sqrt(3.f) * Q + FMath::Sqrt(3.f) / 2 * R);
		const float Y = HexSize * (3.0f / 2 * R);
		return FVector(X, Y, 0);
	}

	// �������
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
	// ������������
	void SetCoordinates(const FLFPHexCoordinates& NewCoords) { Coordinates = NewCoords; }

	// ���ø���״̬
	void SetState(bool bWalkable, bool bOccupied);

	// ������ʾ
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
