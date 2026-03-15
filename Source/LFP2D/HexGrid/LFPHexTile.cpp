// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPTerrainDataAsset.h"
#include "PaperSpriteComponent.h"

// Sets default values
ALFPHexTile::ALFPHexTile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// �������������
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// 创建视觉精灵组件
	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
	SpriteComponent->SetupAttachment(RootComponent);

	// 创建装饰精灵组件（在基础精灵上方）
	DecorationSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("DecorationSpriteComponent"));
	DecorationSpriteComponent->SetupAttachment(RootComponent);
	DecorationSpriteComponent->SetRelativeLocation(FVector(0, 0, 0.5f));

	// 创建范围覆盖层精灵（半透明填充，Z=0.3f）
	OverlaySpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("OverlaySpriteComponent"));
	OverlaySpriteComponent->SetupAttachment(RootComponent);
	OverlaySpriteComponent->SetRelativeLocation(FVector(0, 0, 0.3f));
	OverlaySpriteComponent->SetVisibility(false);

	// 创建 6 个边缘精灵组件
	EdgeSpriteComponents.SetNum(6);
	for (int32 i = 0; i < 6; i++)
	{
		FName CompName = *FString::Printf(TEXT("EdgeSprite_%d"), i);
		EdgeSpriteComponents[i] = CreateDefaultSubobject<UPaperSpriteComponent>(CompName);
		EdgeSpriteComponents[i]->SetupAttachment(RootComponent);
		EdgeSpriteComponents[i]->SetVisibility(false);
	}
}

// Called when the game starts or when spawned
void ALFPHexTile::BeginPlay()
{
	Super::BeginPlay();

	// 如果有装饰精灵，应用到装饰组件
	if (DecorationSprite && DecorationSpriteComponent)
	{
		DecorationSpriteComponent->SetSprite(DecorationSprite);
	}
}

// Called every frame
void ALFPHexTile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALFPHexTile::Highlight(bool bActive)
{
	// Todo
}

void ALFPHexTile::InitializeEdgeComponents(UPaperSprite* EdgeSprite, UPaperSprite* OverlaySprite, float InHexSize, float InVerticalScale)
{
	// 设置覆盖层精灵
	if (OverlaySpriteComponent && OverlaySprite)
	{
		OverlaySpriteComponent->SetSprite(OverlaySprite);
		OverlaySpriteComponent->SetVisibility(false);
		OverlaySpriteComponent->SetRelativeRotation(FRotator(0, 0, 90.f));
	}

	// 计算每条边的位置和旋转（平顶六边形，30° 偏移，与 DrawDebugHexagon 一致）
	const float AngleOffset = 30.0f;
	const float HorizontalRadius = InHexSize;
	const float VerticalRadius = InHexSize * InVerticalScale;

	for (int32 i = 0; i < 6; i++)
	{
		if (!EdgeSpriteComponents.IsValidIndex(i) || !EdgeSpriteComponents[i]) continue;

		// 顶点 i 和 顶点 (i+1)%6
		float Angle1 = FMath::DegreesToRadians(60.0f * i + AngleOffset);
		float Angle2 = FMath::DegreesToRadians(60.0f * ((i + 1) % 6) + AngleOffset);

		FVector V1(HorizontalRadius * FMath::Cos(Angle1), VerticalRadius * FMath::Sin(Angle1), 0);
		FVector V2(HorizontalRadius * FMath::Cos(Angle2), VerticalRadius * FMath::Sin(Angle2), 0);

		// 边中点
		FVector Midpoint = (V1 + V2) * 0.5f;
		Midpoint.Z = 1.0f; // 在装饰层之上

		// 边方向角度
		FVector EdgeDir = V2 - V1;
		float EdgeAngle = FMath::RadiansToDegrees(FMath::Atan2(EdgeDir.Y, EdgeDir.X));

		EdgeSpriteComponents[i]->SetSprite(EdgeSprite);
		EdgeSpriteComponents[i]->SetRelativeLocation(Midpoint);
		// Paper2D 精灵在 XY 平面旋转，使用 Yaw（绕 Z 轴）
		EdgeSpriteComponents[i]->SetRelativeRotation(FRotator(0, EdgeAngle, 90.f));
		EdgeSpriteComponents[i]->SetVisibility(false);

		// 根据边长缩放精灵（假设精灵原始宽度覆盖标准边长）
		float EdgeLength = EdgeDir.Size();
		// 缩放将在配置精灵资产后根据实际尺寸调整
	}
}

void ALFPHexTile::ShowEdge(int32 DirIndex, FLinearColor Color)
{
	if (EdgeSpriteComponents.IsValidIndex(DirIndex) && EdgeSpriteComponents[DirIndex])
	{
		EdgeSpriteComponents[DirIndex]->SetSpriteColor(Color);
		EdgeSpriteComponents[DirIndex]->SetVisibility(true);
	}
}

void ALFPHexTile::HideEdge(int32 DirIndex)
{
	if (EdgeSpriteComponents.IsValidIndex(DirIndex) && EdgeSpriteComponents[DirIndex])
	{
		EdgeSpriteComponents[DirIndex]->SetVisibility(false);
	}
}

void ALFPHexTile::ShowRangeOverlay(FLinearColor Color)
{
	if (OverlaySpriteComponent)
	{
		OverlaySpriteComponent->SetSpriteColor(Color);
		OverlaySpriteComponent->SetVisibility(true);
	}
}

void ALFPHexTile::ShowPathOverlay(bool bActive, FLinearColor Color)
{
	if (OverlaySpriteComponent)
	{
		if (bActive)
		{
			OverlaySpriteComponent->SetSpriteColor(Color);
			OverlaySpriteComponent->SetVisibility(true);
		}
		else
		{
			OverlaySpriteComponent->SetVisibility(false);
		}
	}
}

void ALFPHexTile::ClearAllHighlights()
{
	// 隐藏覆盖层
	if (OverlaySpriteComponent)
	{
		OverlaySpriteComponent->SetVisibility(false);
	}

	// 隐藏所有边缘
	for (int32 i = 0; i < EdgeSpriteComponents.Num(); i++)
	{
		if (EdgeSpriteComponents[i])
		{
			EdgeSpriteComponents[i]->SetVisibility(false);
		}
	}
}

bool ALFPHexTile::IsTargetHexInLine(FLFPHexCoordinates Coord)
{
	if (Coord.Q == Coordinates.Q || Coord.R == Coordinates.R || Coord.S == Coordinates.S)
	{
		return true;
	}
	return false;
}

int32 ALFPHexTile::GetMovementCost() const
{
	if (TerrainData)
	{
		return TerrainData->MovementCost;
	}
	return 1; // 默认代价
}

void ALFPHexTile::SetTerrainData(ULFPTerrainDataAsset* InTerrainData)
{
	TerrainData = InTerrainData;
	if (TerrainData)
	{
		// 从地形数据同步属性
		bIsWalkable = TerrainData->bIsWalkable;

		// 设置基础精灵
		if (TerrainData->DefaultSprite)
		{
			DefaultSprite = TerrainData->DefaultSprite;
			SpriteComponent->SetSprite(DefaultSprite);
		}
	}
}

void ALFPHexTile::SetDecorationByID(FName InID, UPaperSprite* InSprite)
{
	DecorationID = InID;
	DecorationSprite = InSprite;
	if (DecorationSpriteComponent)
	{
		DecorationSpriteComponent->SetSprite(InSprite);
	}
}


//void ALFPHexTile::SetSelected(bool bSelect)
//{
//	bIsSelect = bSelect;
//}

