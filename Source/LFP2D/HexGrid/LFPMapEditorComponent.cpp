#include "LFP2D/HexGrid/LFPMapEditorComponent.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPTerrainDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

ULFPMapEditorComponent::ULFPMapEditorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULFPMapEditorComponent::BeginPlay()
{
	Super::BeginPlay();
}

ALFPHexGridManager* ULFPMapEditorComponent::GetGridManager() const
{
	if (!CachedGridManager)
	{
		TArray<AActor*> Managers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), Managers);
		if (Managers.Num() > 0)
		{
			CachedGridManager = Cast<ALFPHexGridManager>(Managers[0]);
		}
	}
	return CachedGridManager;
}

FString ULFPMapEditorComponent::GetMapSaveDirectory() const
{
	return FPaths::ProjectSavedDir() / TEXT("Maps");
}

void ULFPMapEditorComponent::ToggleEditorMode()
{
	bEditorActive = !bEditorActive;

	if (!bEditorActive)
	{
		CurrentTool = ELFPMapEditorTool::MET_None;
	}

	OnEditorModeChanged.Broadcast(bEditorActive);
	UE_LOG(LogTemp, Log, TEXT("地图编辑器: %s"), bEditorActive ? TEXT("开启") : TEXT("关闭"));
}

void ULFPMapEditorComponent::SetCurrentTool(ELFPMapEditorTool Tool)
{
	CurrentTool = Tool;
	OnEditorToolChanged.Broadcast(CurrentTool);
}

void ULFPMapEditorComponent::ApplyToolToTile(ALFPHexTile* Tile)
{
	if (!Tile || !bEditorActive) return;

	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return;

	switch (CurrentTool)
	{
	case ELFPMapEditorTool::MET_Terrain:
		if (ULFPTerrainDataAsset* TerrainDA = GM->GetTerrainDataForType(BrushTerrainType))
		{
			Tile->SetTerrainData(TerrainDA);
		}
		break;

	case ELFPMapEditorTool::MET_Decoration:
		Tile->SetDecorationByID(BrushDecorationID, GM->GetDecorationSprite(BrushDecorationID));
		break;

	case ELFPMapEditorTool::MET_SpawnPoint:
		Tile->SpawnFaction = BrushSpawnFaction;
		Tile->SpawnIndex = BrushSpawnIndex;
		break;

	case ELFPMapEditorTool::MET_Event:
		Tile->EventTag = BrushEventTag;
		break;

	case ELFPMapEditorTool::MET_RemoveTile:
		{
			FLFPHexCoordinates Coord = Tile->GetCoordinates();
			GM->RemoveTile(Coord.Q, Coord.R);
		}
		break;

	default:
		break;
	}
}

void ULFPMapEditorComponent::ApplyToolToCoord(int32 Q, int32 R)
{
	if (!bEditorActive) return;

	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return;

	if (CurrentTool == ELFPMapEditorTool::MET_AddTile)
	{
		GM->AddTile(Q, R);
	}
}

bool ULFPMapEditorComponent::SaveMap(const FString& FileName)
{
	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return false;

	FString FullPath = GetMapSaveDirectory() / FileName + TEXT(".csv");
	return GM->SaveMapToCSV(FullPath);
}

bool ULFPMapEditorComponent::LoadMap(const FString& FileName)
{
	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return false;

	FString FullPath = GetMapSaveDirectory() / FileName + TEXT(".csv");
	return GM->LoadMapFromCSV(FullPath);
}

TArray<FString> ULFPMapEditorComponent::GetSavedMapList()
{
	TArray<FString> MapFiles;
	FString SearchPath = GetMapSaveDirectory() / TEXT("*.csv");
	IFileManager::Get().FindFiles(MapFiles, *SearchPath, true, false);

	// 去掉 .csv 后缀
	for (FString& File : MapFiles)
	{
		File = FPaths::GetBaseFilename(File);
	}
	return MapFiles;
}

void ULFPMapEditorComponent::NewMap(int32 Width, int32 Height)
{
	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return;

	GM->GenerateGrid(Width, Height);
	UE_LOG(LogTemp, Log, TEXT("新建地图: %dx%d"), Width, Height);
}
