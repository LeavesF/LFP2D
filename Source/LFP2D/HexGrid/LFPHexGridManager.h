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
    // 生成网格
    void GenerateGrid(int32 GridWidth, int32 GridHeight);

    // 获取相邻格子
    TArray<ALFPHexTile*> GetNeighbors(const FLFPHexCoordinates& Coords) const;

    // A*寻路算法
    TArray<ALFPHexTile*> FindPath(ALFPHexTile* Start, ALFPHexTile* End);

    // 获取可移动范围
    TArray<ALFPHexTile*> GetMovementRange(ALFPHexTile* StartTile, int32 MoveRange);

private:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<ALFPHexTile> HexTileClass;

    UPROPERTY(EditAnywhere)
    float HexSize = 100.0f;

    TMap<FIntPoint, ALFPHexTile*> GridMap; // 使用(Q,R)作为键
};
