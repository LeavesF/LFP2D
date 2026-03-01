// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPMapData.h"
#include "LFPHexGridManager.generated.h"

class ULFPTerrainDataAsset;
class UDataTable;
class UPaperSprite;


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

	// 公共参数 Getter
	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	float GetHexSize() const { return HexSize; }

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	float GetVerticalScale() const { return VerticalScale; }

	// ============== 地图数据驱动 ==============

	// 清除所有格子
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	void ClearGrid();

	// 在指定坐标生成单个格子
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	ALFPHexTile* SpawnTileAt(int32 Q, int32 R, ELFPTerrainType TerrainType,
		FName DecorationID = NAME_None,
		ELFPSpawnFaction SpawnFaction = ELFPSpawnFaction::SF_None,
		int32 SpawnIndex = 0,
		FGameplayTag EventTag = FGameplayTag());

	// 添加格子（默认地形）
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	ALFPHexTile* AddTile(int32 Q, int32 R);

	// 移除格子
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	bool RemoveTile(int32 Q, int32 R);

	// 从 DataTable 加载地图
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	void LoadMapFromDataTable(UDataTable* InMapData);

	// 从 CSV 文件加载地图
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	bool LoadMapFromCSV(const FString& CSVFilePath);

	// 保存当前地图到 CSV 文件
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	bool SaveMapToCSV(const FString& CSVFilePath);

	// 导出当前地图数据
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	TArray<FLFPMapTileRow> ExportMapData() const;

	// 查询地形数据资产
	UFUNCTION(BlueprintPure, Category = "Map Data")
	ULFPTerrainDataAsset* GetTerrainDataForType(ELFPTerrainType TerrainType) const;

	// 查询装饰精灵
	UFUNCTION(BlueprintPure, Category = "Map Data")
	UPaperSprite* GetDecorationSprite(FName DecorationID) const;

	// 获取指定阵营出生点
	UFUNCTION(BlueprintCallable, Category = "Map Data")
	TArray<ALFPHexTile*> GetSpawnPoints(ELFPSpawnFaction Faction) const;

	// 获取 GridMap 只读引用
	const TMap<FIntPoint, ALFPHexTile*>& GetGridMap() const { return GridMap; }
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

	// 默认地形数据（没有单独配置的格子使用此数据）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TObjectPtr<ULFPTerrainDataAsset> DefaultTerrainData;

	// 地形注册表：地形类型枚举 → 地形数据资产（在蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Registry")
	TMap<ELFPTerrainType, TObjectPtr<ULFPTerrainDataAsset>> TerrainRegistry;

	// 装饰注册表：装饰 ID → 精灵（在蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data|Registry")
	TMap<FName, TObjectPtr<UPaperSprite>> DecorationRegistry;

	// 预设地图数据表（BeginPlay 时自动加载）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TObjectPtr<UDataTable> MapDataTable;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebug = true;

private:
	TMap<FIntPoint, ALFPHexTile*> GridMap;

	void DrawDebugHexagon(const FVector& Center, FColor Color) const;

    void DrawDebugPath(const TArray<ALFPHexTile*>& Path, bool bPathFound);

	// 六边形方向数组（静态常量）
	static const TArray<FLFPHexCoordinates> HexDirections;

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
