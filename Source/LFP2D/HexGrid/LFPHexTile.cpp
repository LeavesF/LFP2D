// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/HexGrid/LFPHexTile.h"
#include "PaperSpriteComponent.h"

// Sets default values
ALFPHexTile::ALFPHexTile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 创建根场景组件
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// 创建并附加精灵组件到根组件
	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
	SpriteComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ALFPHexTile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALFPHexTile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALFPHexTile::SetPathHighlight(bool bActive)
{
	if (bActive && PathSprite)
	{
		SpriteComponent->SetSprite(PathSprite);
	}
	else if (DefaultSprite)
	{
		SpriteComponent->SetSprite(DefaultSprite);
	}
}

//void ALFPHexTile::SetSelected(bool bSelect)
//{
//	bIsSelect = bSelect;
//}

