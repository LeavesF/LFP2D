// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFPHexGridManager.generated.h"

UCLASS()
class LFP2D_API ALFPHexGridManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALFPHexGridManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    void GenerateGrid(int32 Width, int32 Height);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	TArray<ALFPHexTile*> GetNeighbors(const FLFPHexCoordinates& Coords) const;

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	TArray<ALFPHexTile*> FindPath(ALFPHexTile* Start, ALFPHexTile* End);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	TArray<ALFPHexTile*> GetMovementRange(ALFPHexTile* StartTile, int32 MoveRange);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	ALFPHexTile* GetTileAtCoordinates(const FLFPHexCoordinates& Coords) const;


    UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    void SetVerticalScale(float Scale) { VerticalScale = Scale; }

	// ���Կ���
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void SetDebugEnabled(bool bEnabled) { bDrawDebug = bEnabled; }
protected:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<ALFPHexTile> HexTileClass;

	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	float HexSize;

	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	int32 GridWidth;

	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	int32 GridHeight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float VerticalScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebug = true;

private:
	TMap<FIntPoint, ALFPHexTile*> GridMap;

	void DrawDebugHexagon(const FVector& Center, FColor Color) const;

    void DrawDebugPath(const TArray<ALFPHexTile*>& Path, bool bPathFound);

	// �����η����� (��̬����)
	static const TArray<FLFPHexCoordinates> HexDirections;

    //// Ѱ·�ڵ�ṹ������A*�㷨��
    //struct FPathNode
    //{
    //    ALFPHexTile* Tile;
    //    float GScore; // ����㵽��ǰ�ڵ��ʵ�ʴ���
    //    float FScore; // GScore + ����ʽ����ֵ
    //    FPathNode* Parent;

    //    FPathNode(ALFPHexTile* InTile, float InG, float InF, FPathNode* InParent)
    //        : Tile(InTile), GScore(InG), FScore(InF), Parent(InParent) {}

    //    // �������ȶ��бȽ�
    //    bool operator<(const FPathNode& Other) const
    //    {
    //        return FScore > Other.FScore; // ע�⣺���ȶ��������ѣ��������ﷴת�Ƚ�
    //    }
    //};

    //// �Զ������ȶ��бȽϺ���
    //struct FComparePathNode
    //{
    //    bool operator()(const FPathNode* A, const FPathNode* B) const
    //    {
    //        return A->FScore > B->FScore;
    //    }
    //};

    //// �洢���е�������Ƭ
    //UPROPERTY()
    //TArray<ALFPHexTile*> GridTiles;

    //// ����Tile��ȡ����Ĺ�ϣ����
    //FORCEINLINE uint32 GetTypeHash(const ALFPHexTile* Tile) const
    //{
    //    return PointerHash(Tile);
    //}
public:
    UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    ALFPHexTile* GetHexTileUnderCursor(const FVector2D& ScreenPosition, APlayerController* PlayerController);

    UFUNCTION(BlueprintPure, Category = "Hex Grid")
    FVector2D WorldToHexGridPosition(const FVector& WorldPosition, float InHexSize, float InVerticalScale = 1.0f);

    UFUNCTION(BlueprintPure, Category = "Hex Grid")
    FLFPHexCoordinates PixelToHexCoordinates(const FVector2D& PixelPosition, float InHexSize);

    UFUNCTION(BlueprintPure, Category = "Hex Grid")
    FLFPHexCoordinates RoundToHex(float q, float r);

protected:
    // ���������α߽�
    UFUNCTION(BlueprintPure, Category = "Hex Grid")
    bool IsPointInHexagon(const FVector2D& Point, const FVector2D& Center, float InHexSize);
};
