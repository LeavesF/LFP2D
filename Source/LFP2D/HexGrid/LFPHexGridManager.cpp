// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

// �����η������� (ƽ�������β���)
const TArray<FLFPHexCoordinates> ALFPHexGridManager::HexDirections = {
	FLFPHexCoordinates(1, 0),   // ��
	FLFPHexCoordinates(1, -1),  // ����
	FLFPHexCoordinates(0, -1),  // ����
	FLFPHexCoordinates(-1, 0),  // ��
	FLFPHexCoordinates(-1, 1),  // ����
	FLFPHexCoordinates(0, 1)    // ����
};

// Sets default values
ALFPHexGridManager::ALFPHexGridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HexSize = 100.0f;
	GridWidth = 10;
	GridHeight = 10;
}

// Called when the game starts or when spawned
void ALFPHexGridManager::BeginPlay()
{
	Super::BeginPlay();

	GenerateGrid(GridWidth, GridHeight);
}

// Called every frame
void ALFPHexGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ���Ի�������
	if (bDrawDebug)
	{
		for (auto& TilePair : GridMap)
		{
			if (TilePair.Value)
			{
				FVector Center = TilePair.Value->GetActorLocation();
				DrawDebugHexagon(Center, FColor::Green);

				// ��ʾ����
				FLFPHexCoordinates Coords = TilePair.Value->GetCoordinates();
				FString CoordText = FString::Printf(TEXT("(%d,%d)"), Coords.Q, Coords.R);
				DrawDebugString(GetWorld(), Center + FVector(0, 0, 20), CoordText, nullptr, FColor::White, 0, true);
			}
		}
	}
}

void ALFPHexGridManager::GenerateGrid(int32 Width, int32 Height)
{
	// �����������
	for (auto& Tile : GridMap)
	{
		if (Tile.Value)
		{
			Tile.Value->Destroy();
		}
	}
	GridMap.Empty();

	// ����������
	for (int32 r = 0; r < Height; r++)
	{
		for (int32 q = 0; q < Width; q++)
		{
			// ����ƫ������ - ƽ�������β���
			int32 offsetQ = q - FMath::FloorToInt(r / 2.0f);

			// ��������������
			FLFPHexCoordinates HexCoord(offsetQ, r);

			// ��������λ��
			FVector WorldLocation = HexCoord.ToWorldLocation(HexSize) + GetActorLocation();

			// ���ɸ���
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			ALFPHexTile* NewTile = GetWorld()->SpawnActor<ALFPHexTile>(
				HexTileClass,
				WorldLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);

			if (NewTile)
			{
				NewTile->SetCoordinates(HexCoord);
				NewTile->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

				// ��ӵ�����ӳ�� (ʹ��Q,R��Ϊ��)
				FIntPoint Key(HexCoord.Q, HexCoord.R);
				GridMap.Add(Key, NewTile);
			}
		}
	}
}

TArray<ALFPHexTile*> ALFPHexGridManager::GetNeighbors(const FLFPHexCoordinates& Coords) const
{
	TArray<ALFPHexTile*> Neighbors;

	for (const FLFPHexCoordinates& Dir : HexDirections)
	{
		FLFPHexCoordinates NeighborCoords(Coords.Q + Dir.Q, Coords.R + Dir.R);
		FIntPoint Key(NeighborCoords.Q, NeighborCoords.R);

		if (ALFPHexTile* const* FoundTile = GridMap.Find(Key))
		{
			Neighbors.Add(*FoundTile);
		}
	}

	return Neighbors;
}

TArray<ALFPHexTile*> ALFPHexGridManager::GetMovementRange(ALFPHexTile* StartTile, int32 MoveRange)
{
	TArray<ALFPHexTile*> ReachableTiles;
	if (!StartTile || MoveRange <= 0) return ReachableTiles;

	// BFS���ݽṹ
	TMap<ALFPHexTile*, int32> MoveCosts;
	TQueue<ALFPHexTile*> TileQueue;

	TileQueue.Enqueue(StartTile);
	MoveCosts.Add(StartTile, 0);
	ReachableTiles.Add(StartTile);

	while (!TileQueue.IsEmpty())
	{
		ALFPHexTile* CurrentTile;
		TileQueue.Dequeue(CurrentTile);
		int32 CurrentCost = MoveCosts[CurrentTile];

		// ��ȡ�����ھ�
		for (ALFPHexTile* Neighbor : GetNeighbors(CurrentTile->GetCoordinates()))
		{
			if (!Neighbor || !Neighbor->IsWalkable()) continue;

			// �������ƶ�����
			const int32 NewCost = CurrentCost + 1;

			// �����δ���ʻ��ҵ�����·��
			if (!MoveCosts.Contains(Neighbor) || NewCost < MoveCosts[Neighbor])
			{
				MoveCosts.Add(Neighbor, NewCost);

				// ������ƶ���Χ��
				if (NewCost <= MoveRange)
				{
					ReachableTiles.Add(Neighbor);
					TileQueue.Enqueue(Neighbor);
				}
			}
		}
	}

	return ReachableTiles;
}

ALFPHexTile* ALFPHexGridManager::GetTileAtCoordinates(const FLFPHexCoordinates& Coords) const
{
	FIntPoint Key(Coords.Q, Coords.R);
	if (ALFPHexTile* const* FoundTile = GridMap.Find(Key))
	{
		return *FoundTile;
	}
	return nullptr;
}

void ALFPHexGridManager::DrawDebugHexagon(const FVector& Center, FColor Color) const
{
	const float AngleStep = 60.0f;
	const float Radius = HexSize;

	// ��� 30�� ƫ�ƣ�ʹ��������ȷ����
	const float AngleOffset = 30.0f;

	TArray<FVector> Points;
	for (int32 i = 0; i < 6; i++)
	{
		// ��� 30�� ƫ��
		float Angle = FMath::DegreesToRadians(AngleStep * i + AngleOffset);
		FVector Point(
			Center.X + Radius * FMath::Cos(Angle),
			Center.Y + Radius * FMath::Sin(Angle),
			Center.Z
		);
		Points.Add(Point);
	}

	// ���ӵ��γ�������
	for (int32 i = 0; i < 6; i++)
	{
		int32 NextIndex = (i + 1) % 6;
		DrawDebugLine(GetWorld(), Points[i], Points[NextIndex], Color, false, -1, 0, 2);
	}
}


