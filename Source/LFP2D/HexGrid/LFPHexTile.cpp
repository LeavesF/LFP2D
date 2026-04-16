// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPTerrainDataAsset.h"
#include "PaperSpriteComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

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

	// 创建 6 个过渡精灵组件（Z=0.15，介于基础地形和覆盖层之间）
	TransitionComponents.SetNum(6);
	for (int32 i = 0; i < 6; i++)
	{
		FName CompName = *FString::Printf(TEXT("TransitionSprite_%d"), i);
		TransitionComponents[i] = CreateDefaultSubobject<UPaperSpriteComponent>(CompName);
		TransitionComponents[i]->SetupAttachment(RootComponent);
		TransitionComponents[i]->SetRelativeLocation(FVector(0, 0, 0.15f));
		TransitionComponents[i]->SetVisibility(false);
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

void ALFPHexTile::ShowEdge(int32 DirIndex, FLinearColor Color, EUnitRange HexRangeType)
{
	if (EdgeSpriteComponents.IsValidIndex(DirIndex) && EdgeSpriteComponents[DirIndex])
	{
		int32 SortPriority = 0;
		switch (HexRangeType)
		{
		case EUnitRange::UR_Move:
			SortPriority = MoveEdgeTranslucencySortPriority;
			break;

		case EUnitRange::UR_SkillRelease:
			SortPriority = SkillReleaseEdgeTranslucencySortPriority;
			break;

		case EUnitRange::UR_SkillEffect:
			SortPriority = SkillEffectEdgeTranslucencySortPriority;
			break;

        case EUnitRange::UR_Enemy_Move:
            SortPriority = MoveEdgeTranslucencySortPriority;
            break;

        case EUnitRange::UR_Enemy_SkillRelease:
            SortPriority = SkillReleaseEdgeTranslucencySortPriority;
            break;

        case EUnitRange::UR_Enemy_SkillEffect:
            SortPriority = SkillEffectEdgeTranslucencySortPriority;
            break;
		default:
			break;
		}

		EdgeSpriteComponents[DirIndex]->TranslucencySortPriority = SortPriority;
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

		// 更新基础材质的纹理参数
		if (BaseMID && TerrainData->TerrainTexture)
		{
			BaseMID->SetTextureParameterValue(TEXT("TerrainTexture"), TerrainData->TerrainTexture);
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

// ============== 地形过渡系统实现 ==============

void ALFPHexTile::InitializeBaseMaterial(UMaterialInterface* BaseTerrainMat,
	float TextureScale, float InHexMaskScale, float InHexMaskYScale)
{
	if (!BaseTerrainMat) return;

	BaseMID = UMaterialInstanceDynamic::Create(BaseTerrainMat, this);
	if (BaseMID)
	{
		BaseMID->SetScalarParameterValue(TEXT("TextureScale"), TextureScale);
		BaseMID->SetScalarParameterValue(TEXT("HexMaskScale"), InHexMaskScale);
		BaseMID->SetScalarParameterValue(TEXT("HexMaskYScale"), InHexMaskYScale);
		SpriteComponent->SetMaterial(0, BaseMID);
	}
}

void ALFPHexTile::InitializeTransitionComponents(UPaperSprite* HexSprite,
	UMaterialInterface* TransitionMat,
	float TextureScale, float InHexMaskScale, float InHexMaskYScale)
{
	if (!TransitionMat || !HexSprite) return;

	// 为每个过渡组件创建动态材质
	TransitionMIDs.SetNum(6);

	// 方向到角度的映射（对应 HexDirections: 东0°, 东北60°, 西北120°, 西180°, 西南240°, 东南300°）
	const float DirAngles[6] = { 0.f, -60.f, -120.f, -180.f, -240.f, -300.f };

	for (int32 i = 0; i < 6; i++)
	{
		if (!TransitionComponents.IsValidIndex(i) || !TransitionComponents[i]) continue;

		// 设置精灵（使用相同的六角精灵覆盖整个格子区域）
		TransitionComponents[i]->SetSprite(HexSprite);

		// 创建动态材质实例
		TransitionMIDs[i] = UMaterialInstanceDynamic::Create(TransitionMat, this);
		if (TransitionMIDs[i])
		{
			TransitionMIDs[i]->SetScalarParameterValue(TEXT("TextureScale"), TextureScale);
			TransitionMIDs[i]->SetScalarParameterValue(TEXT("MaskAngle"), DirAngles[i]);
			TransitionMIDs[i]->SetScalarParameterValue(TEXT("HexMaskScale"), InHexMaskScale);
			TransitionMIDs[i]->SetScalarParameterValue(TEXT("HexMaskYScale"), InHexMaskYScale);
			TransitionComponents[i]->SetMaterial(0, TransitionMIDs[i]);
		}

		TransitionComponents[i]->SetVisibility(false);
	}
}

void ALFPHexTile::UpdateBaseMaterial(UTexture2D* InTerrainTexture, float TextureScale)
{
	if (BaseMID && InTerrainTexture)
	{
		BaseMID->SetTextureParameterValue(TEXT("TerrainTexture"), InTerrainTexture);
		BaseMID->SetScalarParameterValue(TEXT("TextureScale"), TextureScale);
	}
}

void ALFPHexTile::ShowTransition(int32 DirIndex, UTexture2D* NeighborTexture, float TextureScale)
{
	if (!TransitionComponents.IsValidIndex(DirIndex) || !TransitionComponents[DirIndex]) return;
	if (!TransitionMIDs.IsValidIndex(DirIndex) || !TransitionMIDs[DirIndex]) return;
	if (!NeighborTexture) return;

	TransitionMIDs[DirIndex]->SetTextureParameterValue(TEXT("NeighborTexture"), NeighborTexture);
	TransitionMIDs[DirIndex]->SetScalarParameterValue(TEXT("TextureScale"), TextureScale);
	TransitionComponents[DirIndex]->SetVisibility(true);
}

void ALFPHexTile::HideTransition(int32 DirIndex)
{
	if (TransitionComponents.IsValidIndex(DirIndex) && TransitionComponents[DirIndex])
	{
		TransitionComponents[DirIndex]->SetVisibility(false);
	}
}

void ALFPHexTile::ClearAllTransitions()
{
	for (int32 i = 0; i < TransitionComponents.Num(); i++)
	{
		if (TransitionComponents[i])
		{
			TransitionComponents[i]->SetVisibility(false);
		}
	}
}
