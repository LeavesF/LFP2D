// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/HexGrid/LFPHexGridManager.h"

// Sets default values
ALFPHexGridManager::ALFPHexGridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ALFPHexGridManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALFPHexGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//TArray<ALFPHexTile*> ALFPHexGridManager::FindPath(ALFPHexTile* Start, ALFPHexTile* End)
//{
//    TArray<ALFPHexTile*> Path;
//    TMap<ALFPHexTile*, ALFPHexTile*> CameFrom;
//    TMap<ALFPHexTile*, float> CostSoFar;
//
//    TArray<ALFPHexTile*> OpenSet;
//    OpenSet.HeapPush(Start, [&](ALFPHexTile* A, ALFPHexTile* B) {
//        return FLFPHexCoordinates::Distance(A->GetCoordinates(), End->GetCoordinates()) <
//            FLFPHexCoordinates::Distance(B->GetCoordinates(), End->GetCoordinates());
//        });
//
//    CameFrom.Add(Start, nullptr);
//    CostSoFar.Add(Start, 0);
//
//    while (OpenSet.Num() > 0)
//    {
//        ALFPHexTile* Current;
//        OpenSet.HeapPop(Current, [](...) {}, true);
//
//        if (Current == End) {
//            // 重建路径
//            while (Current != Start) {
//                Path.Add(Current);
//                Current = CameFrom[Current];
//            }
//            Algo::Reverse(Path);
//            break;
//        }
//
//        for (ALFPHexTile* Neighbor : GetNeighbors(Current->GetCoordinates()))
//        {
//            if (!Neighbor || !Neighbor->IsWalkable()) continue;
//
//            const float NewCost = CostSoFar[Current] + 1; // 假设每格成本为1
//            if (!CostSoFar.Contains(Neighbor) || NewCost < CostSoFar[Neighbor])
//            {
//                CostSoFar.Add(Neighbor, NewCost);
//                const float Priority = NewCost + FLFPHexCoordinates::Distance(Neighbor->GetCoordinates(), End->GetCoordinates());
//                OpenSet.HeapPush(Neighbor, [&](...) { return Priority; });
//                CameFrom.Add(Neighbor, Current);
//            }
//        }
//    }
//    return Path;
//}

