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
class UMaterialInterface;


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
	// ============== 地形过渡系统 ==============

	// 基础地形材质（World-Space UV）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
	TObjectPtr<UMaterialInterface> TerrainBaseMaterial;

	// 过渡层材质（Alpha 遮罩混合）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
	TObjectPtr<UMaterialInterface> TerrainTransitionMaterial;

	// 过渡层精灵（六角形，覆盖整个格子区域）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
	TObjectPtr<UPaperSprite> TransitionHexSprite;

	// 纹理缩放（值越大纹理显示越大、重复越少。建议为 HexSize 的 10~20 倍）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition", meta = (ClampMin = "0.1"))
	float TerrainTextureScale = 1500.0f;

	// 是否启用过渡效果
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition")
	bool bEnableTerrainTransition = true;

	// 六角遮罩大小（1.0 = 精确匹配精灵边界，>1 稍大可避免缝隙）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float HexMaskScale = 1.05f;

	// 六角遮罩 Y 轴缩放（调整六角形纵向比例，1.0 = 正六角形）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Transition", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float HexMaskYScale = 1.0f;

	// 更新单个格子的所有过渡
	UFUNCTION(BlueprintCallable, Category = "Terrain|Transition")
	void UpdateTileTransitions(ALFPHexTile* Tile);

	// 更新所有格子的过渡
	UFUNCTION(BlueprintCallable, Category = "Terrain|Transition")
	void UpdateAllTransitions();

	// 更新指定坐标及其邻居的过渡（用于编辑器修改后）
	UFUNCTION(BlueprintCallable, Category = "Terrain|Transition")
	void UpdateTransitionsAround(int32 Q, int32 R);

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
	// ============== 范围高亮系统 ==============

	// 边缘高亮精灵（通用白色线段，通过颜色区分类型，在蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	TObjectPtr<UPaperSprite> EdgeHighlightSprite;

	// 范围覆盖层精灵（白色六边形，半透明填充，在蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	TObjectPtr<UPaperSprite> RangeOverlaySprite;

	// ---- 颜色配置（蓝图可调） ----

	// 移动范围 - 边缘颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Move")
	FLinearColor MoveEdgeColor = FLinearColor(0.2f, 0.5f, 1.0f, 0.8f);

	// 移动范围 - 覆盖层颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Move")
	FLinearColor MoveOverlayColor = FLinearColor(0.2f, 0.5f, 1.0f, 0.25f);

	// 攻击范围 - 边缘颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Attack")
	FLinearColor AttackEdgeColor = FLinearColor(1.0f, 0.2f, 0.2f, 0.8f);

	// 攻击范围 - 覆盖层颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Attack")
	FLinearColor AttackOverlayColor = FLinearColor(1.0f, 0.2f, 0.2f, 0.25f);

	// 技能效果范围 - 边缘颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Skill")
	FLinearColor SkillEdgeColor = FLinearColor(0.2f, 1.0f, 0.4f, 0.8f);

	// 技能效果范围 - 覆盖层颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Skill")
	FLinearColor SkillOverlayColor = FLinearColor(0.2f, 1.0f, 0.4f, 0.25f);

	// 路径 - 覆盖层颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight|Path")
	FLinearColor PathOverlayColor = FLinearColor(1.0f, 0.9f, 0.3f, 0.35f);

	// 根据范围类型获取对应颜色
	FLinearColor GetEdgeColorForRange(EUnitRange HexRangeType) const;
	FLinearColor GetOverlayColorForRange(EUnitRange HexRangeType) const;

	// 显示范围高亮（带边界描边检测）
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ShowRangeHighlight(const TArray<ALFPHexTile*>& RangeTiles, EUnitRange HexRangeType);

	// 显示范围高亮（坐标版）
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ShowRangeHighlightByCoords(const TArray<FLFPHexCoordinates>& Coords, EUnitRange HexRangeType);

	// 显示路径高亮
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ShowPathHighlight(const TArray<ALFPHexTile*>& PathTiles);

	// 清除路径高亮（保留范围高亮）
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ClearPathHighlight();

	// 清除所有高亮
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ClearAllHighlights();

private:
	// 当前高亮的格子缓存
	TArray<ALFPHexTile*> CurrentHighlightedTiles;
	TArray<ALFPHexTile*> CurrentPathTiles;
	EUnitRange CurrentHighlightRange = EUnitRange::UR_Default;
};
