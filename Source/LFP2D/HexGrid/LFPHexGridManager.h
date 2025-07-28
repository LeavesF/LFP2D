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

	/*UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	TArray<ALFPHexTile*> FindPath(ALFPHexTile* Start, ALFPHexTile* End);*/

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	TArray<ALFPHexTile*> GetMovementRange(ALFPHexTile* StartTile, int32 MoveRange);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	ALFPHexTile* GetTileAtCoordinates(const FLFPHexCoordinates& Coords) const;

	// 调试开关
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void SetDebugEnabled(bool bEnabled) { bDrawDebug = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	TArray<ALFPHexTile*> FindPath(ALFPHexTile* Start, ALFPHexTile* End);
private:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<ALFPHexTile> HexTileClass;

	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	float HexSize;

	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	int32 GridWidth;

	UPROPERTY(EditAnywhere, Category = "Hex Grid")
	int32 GridHeight;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebug = true;

private:
	TMap<FIntPoint, ALFPHexTile*> GridMap;

	void DrawDebugHexagon(const FVector& Center, FColor Color) const;

	// 六边形方向定义 (静态常量)
	static const TArray<FLFPHexCoordinates> HexDirections;
};
