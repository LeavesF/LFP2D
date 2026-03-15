// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Math/IntPoint.h"
#include "LFPHexTile.generated.h"

class UPaperSpriteComponent;
class UPaperSprite;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;
class ALFPTacticsUnit;
class ULFPTerrainDataAsset;
enum class ELFPSpawnFaction : uint8;

USTRUCT(BlueprintType)
struct FLFPHexCoordinates
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexCoordinates")
	int32 Q; // 立方坐标 q

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexCoordinates")
	int32 R; // 立方坐标 r

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexCoordinates")
	int32 S; // 立方坐标 s（自动计算）

	FLFPHexCoordinates() : Q(0), R(0), S(0) {}

	FLFPHexCoordinates(int32 q, int32 r) : Q(q), R(r), S(-q - r) {}

	// 坐标转换函数
	FVector ToWorldLocation(float Size, float VerticalScale = 1.0f) const
	{
		// 平顶六边形布局
		const float HorizontalSpacing = Size * FMath::Sqrt(3.f); // 水平间距
		const float VerticalSpacing = Size * 1.5f * VerticalScale; // 垂直间距（应用缩放）

		float X = HorizontalSpacing * (Q + R * 0.5f);
		float Y = VerticalSpacing * R;

		return FVector(X, Y, 0);
	}

	// 计算距离
	static int32 Distance(const FLFPHexCoordinates& A, const FLFPHexCoordinates& B)
	{
		return (FMath::Abs(A.Q - B.Q) +
			FMath::Abs(A.R - B.R) +
			FMath::Abs(A.S - B.S)) / 2;
	}

	// 获取所有相邻坐标
	TArray<FLFPHexCoordinates> GetNeighbors() const
	{
		// 六边形的邻居方向
		static const TArray<FLFPHexCoordinates> Directions = {
			FLFPHexCoordinates(+1,  0), // 东
			FLFPHexCoordinates(+1, -1), // 东北
			FLFPHexCoordinates(0, -1), // 西北
			FLFPHexCoordinates(-1,  0), // 西
			FLFPHexCoordinates(-1, +1), // 西南
			FLFPHexCoordinates(0, +1)  // 东南
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
		// 找出距离点最近的一个锚点
		for (int i = 0; i < 3; i++)
		{
			// 计算欧式距离
			//UE_LOG(LogTemp, Warning, TEXT("anchor:(%f,%f)"), anchors[i].X, anchors[i].Y);
			distance = FVector2D::Distance(FVector2D(WorldLocation_X, WorldLocation_Y), FVector2D(anchors[i].X, anchors[i].Y));
			//UE_LOG(LogTemp, Warning, TEXT("distance:(%f)"), distance);
			// 如果已经十分确定了
			if (distance < minDistanceLimit)
			{
				index = i;
				break;
			}

			// 记录最小距离和索引
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

// 单位显示枚举
UENUM(BlueprintType)
enum class EUnitRange : uint8
{
	UR_Default     UMETA(DisplayName = "Default"),
	UR_Move     UMETA(DisplayName = "Move"),
	UR_Attack    UMETA(DisplayName = "Attack"),
	UR_SkillEffect    UMETA(DisplayName = "SkillEffect")
};

// 玩家操作状态
UENUM(BlueprintType)
enum class EPlayControlState : uint8
{
	MoveState,
	SkillReleaseState
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
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	FLFPHexCoordinates GetCoordinates() { return Coordinates; }
	// 设置六边形坐标
	void SetCoordinates(const FLFPHexCoordinates& NewCoords) { Coordinates = NewCoords; }

	bool IsWalkable() { return bIsWalkable; }
	bool IsOccupied() { return bIsOccupied; }

	void SetIsWalkable(bool bInIsWalkable) { bIsWalkable = bInIsWalkable; }
	void SetIsOccupied(bool bInIsOccupied) { bIsOccupied = bInIsOccupied; }

	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	ALFPTacticsUnit* GetUnitOnTile() { return CurrentUnit; }

	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
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

	// 地形数据资产引用
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TObjectPtr<ULFPTerrainDataAsset> TerrainData;

	TObjectPtr<ALFPTacticsUnit> CurrentUnit;

public:
	// 获取移动代价（从地形数据获取，无数据默认返回 1）
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	int32 GetMovementCost() const;

	// 设置地形数据（同步可行走性和精灵）
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetTerrainData(ULFPTerrainDataAsset* InTerrainData);

	// 获取地形数据
	UFUNCTION(BlueprintPure, Category = "Hex Tile")
	ULFPTerrainDataAsset* GetTerrainData() const { return TerrainData; }

	// 装饰精灵组件（叠加在基础地形上方，纯视觉）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> DecorationSpriteComponent;

	// 装饰精灵（编辑器中可配置，纯视觉无游戏效果）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decoration")
	TObjectPtr<UPaperSprite> DecorationSprite;

	// ============== 地图数据字段 ==============

	// 出生点阵营
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
	ELFPSpawnFaction SpawnFaction;

	// 出生点顺序索引（同一阵营内的出生顺序，0 = 非出生点）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
	int32 SpawnIndex = 0;

	// 事件标签（空 = 无事件）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
	FGameplayTag EventTag;

	// 装饰标识（对应装饰注册表中的 Key）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Data")
	FName DecorationID = NAME_None;

	// 运行时设置装饰精灵（通过 ID 和精灵引用）
	UFUNCTION(BlueprintCallable, Category = "Hex Tile")
	void SetDecorationByID(FName InID, UPaperSprite* InSprite);

	// ============== 高亮系统 ==============

	// 初始化边缘和覆盖层组件（由 GridManager 在生成格子后调用）
	void InitializeEdgeComponents(UPaperSprite* EdgeSprite, UPaperSprite* OverlaySprite, float InHexSize, float InVerticalScale);

	// 显示指定方向的边缘高亮（方向索引 0-5，对应 HexDirections）
	void ShowEdge(int32 DirIndex, FLinearColor Color);

	// 隐藏指定方向的边缘高亮
	void HideEdge(int32 DirIndex);

	// 显示范围覆盖层（传入颜色）
	void ShowRangeOverlay(FLinearColor Color);

	// 显示路径覆盖层（传入颜色）
	void ShowPathOverlay(bool bActive, FLinearColor Color = FLinearColor::Transparent);

	// 清除所有高亮（覆盖层 + 所有边缘）
	void ClearAllHighlights();

	// ============== 地形过渡系统 ==============

	// 初始化过渡组件（由 GridManager 在生成格子后调用）
	void InitializeTransitionComponents(UPaperSprite* HexSprite,
		UMaterialInterface* BaseTerrainMat, UMaterialInterface* TransitionMat,
		float TextureScale, float InHexMaskScale, float InHexMaskYScale);

	// 更新基础地形材质纹理参数
	void UpdateBaseMaterial(UTexture2D* InTerrainTexture, float TextureScale);

	// 显示指定方向的过渡（DirIndex 对应 HexDirections 0-5）
	void ShowTransition(int32 DirIndex, UTexture2D* NeighborTexture, float TextureScale);

	// 隐藏指定方向的过渡
	void HideTransition(int32 DirIndex);

	// 清除所有过渡
	void ClearAllTransitions();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UPaperSprite> DefaultSprite;

	// 覆盖层精灵组件（半透明填充）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> OverlaySpriteComponent;

	// 6 个边缘精灵组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UPaperSpriteComponent>> EdgeSpriteComponents;

/////////////////	Math	///////////////
public:
	UFUNCTION(BlueprintCallable, Category = "Hex Math")
	bool IsTargetHexInLine(FLFPHexCoordinates Coord);

	// ============== 地形过渡组件 ==============
protected:
	// 6 个过渡精灵组件（每个方向一个，Z=0.15）
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UPaperSpriteComponent>> TransitionComponents;

	// 基础地形动态材质实例
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BaseMID;

	// 6 个过渡层动态材质实例
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TransitionMIDs;
};
