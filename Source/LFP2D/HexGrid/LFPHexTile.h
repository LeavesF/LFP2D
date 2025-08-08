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

public:

	// ���·����������
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetPathHighlight(bool bActive);

	// ����ƶ���Χ��������
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetMovementHighlight(bool bActive);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPaperSprite> DefaultSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Sprites")
	TObjectPtr<UPaperSprite> PathSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Sprites")
	TObjectPtr<UPaperSprite> MovementRangeSprite;
};
