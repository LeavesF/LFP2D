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

void ALFPHexTile::SetMovementHighlight(bool bActive)
{
	if (bActive && MovementRangeSprite)
	{
		SpriteComponent->SetSprite(MovementRangeSprite);
	}
	else if (DefaultSprite)
	{
		SpriteComponent->SetSprite(DefaultSprite);
	}
	else if (!DefaultSprite)
	{
		SpriteComponent->SetSprite(nullptr);
	}
}

void ALFPHexTile::SetAttackHighlight(bool bActive)
{
	if (bActive && AttackRangeSprite)
	{
		SpriteComponent->SetSprite(AttackRangeSprite);
	}
	else if (DefaultSprite)
	{
		SpriteComponent->SetSprite(DefaultSprite);
	}
	else if (!DefaultSprite)
	{
		SpriteComponent->SetSprite(nullptr);
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

void ALFPHexTile::SetRangeSprite(EUnitRange UnitRange)
{
	switch (UnitRange)
	{
	case EUnitRange::UR_Default:
		SpriteComponent->SetSprite(DefaultSprite);
		break;
	case EUnitRange::UR_Move:
		SpriteComponent->SetSprite(MovementRangeSprite);
		break;
	case EUnitRange::UR_Attack:
		SpriteComponent->SetSprite(AttackRangeSprite);
		break;
	case EUnitRange::UR_SkillEffect:
		SpriteComponent->SetSprite(SkillEffectRangeSprite);
	default:
		break;
	}
}

void ALFPHexTile::SetPathHighlight(bool bActive)
{
	if (bActive && PathSprite)
	{
		SpriteComponent->SetSprite(PathSprite);
	}
	else if (MovementRangeSprite)
	{
		SpriteComponent->SetSprite(MovementRangeSprite);
	}
}

//void ALFPHexTile::SetSelected(bool bSelect)
//{
//	bIsSelect = bSelect;
//}

