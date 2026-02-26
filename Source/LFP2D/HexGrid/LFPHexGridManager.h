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
    FLFPHexCoordinates Coord_AddQ(FLFPHexCoordinates Coord, int32 In_Q)
    {
        return FLFPHexCoordinates(Coord.Q + In_Q, Coord.R);
    }

    UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    FLFPHexCoordinates Coord_AddR(FLFPHexCoordinates Coord, int32 In_R)
    {
        return FLFPHexCoordinates(Coord.Q, Coord.R + In_R);
    }

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	TArray<ALFPHexTile*> FindPath(ALFPHexTile* Start, ALFPHexTile* End);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    TArray<ALFPHexTile*> GetTilesInRange(ALFPHexTile* Center, int32 MaxRange, int32 MinRange = 0);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	ALFPHexTile* GetTileAtCoordinates(const FLFPHexCoordinates& Coords) const;


    UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    void SetVerticalScale(float Scale) { VerticalScale = Scale; }

	// 调试控制
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

	// 六边形方向数组（静态常量）
	static const TArray<FLFPHexCoordinates> HexDirections;

    //// 寻路节点结构体（用于A*算法）
    //struct FPathNode
    //{
    //    ALFPHexTile* Tile;
    //    float GScore; // 从起点到当前节点的实际代价
    //    float FScore; // GScore + 启发式估计值
    //    FPathNode* Parent;

    //    FPathNode(ALFPHexTile* InTile, float InG, float InF, FPathNode* InParent)
    //        : Tile(InTile), GScore(InG), FScore(InF), Parent(InParent) {}

    //    // 用于优先队列比较
    //    bool operator<(const FPathNode& Other) const
    //    {
    //        return FScore > Other.FScore; // 注意：优先队列是最大堆，所以反转比较
    //    }
    //};

    //// 自定义优先队列比较函数
    //struct FComparePathNode
    //{
    //    bool operator()(const FPathNode* A, const FPathNode* B) const
    //    {
    //        return A->FScore > B->FScore;
    //    }
    //};

    //// 存储所有的六角格片
    //UPROPERTY()
    //TArray<ALFPHexTile*> GridTiles;

    //// 根据Tile获取坐标的哈希函数
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
    // 检测点是否在六边形边界内
    UFUNCTION(BlueprintPure, Category = "Hex Grid")
    bool IsPointInHexagon(const FVector2D& Point, const FVector2D& Center, float InHexSize);


public:
    UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    void UpdateGridSpriteWithTiles(EPlayControlState CurrentControlState, TArray<ALFPHexTile*>& Tiles);

    UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    void UpdateGridSpriteWithCoords(EPlayControlState CurrentControlState, TArray<FLFPHexCoordinates>& Coords);

    UFUNCTION(BlueprintCallable, Category = "Hex Grid")
    void ResetGridSprite();
};
