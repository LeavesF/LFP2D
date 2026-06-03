#include "LFP2D/HexGrid/LFPMapEditorComponent.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPTerrainDataAsset.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
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

FString ULFPMapEditorComponent::GetEnemyMapFilePath(const FString& FileName) const
{
	return GetMapSaveDirectory() / (FileName + TEXT("_enemies.csv"));
}

bool ULFPMapEditorComponent::SaveEnemyUnits(const FString& FileName)
{
	FString CSVContent = TEXT("---,Q,R,UnitTypeID\n");

	TArray<FIntPoint> SortedCoords;
	EnemyUnitTypeByCoord.GetKeys(SortedCoords);
	SortedCoords.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		if (A.Y == B.Y)
		{
			return A.X < B.X;
		}
		return A.Y < B.Y;
	});

	for (const FIntPoint& Coord : SortedCoords)
	{
		const FName* UnitTypeID = EnemyUnitTypeByCoord.Find(Coord);
		if (!UnitTypeID || UnitTypeID->IsNone())
		{
			continue;
		}

		const FString RowName = FString::Printf(TEXT("%d_%d"), Coord.X, Coord.Y);
		CSVContent += FString::Printf(TEXT("%s,%d,%d,%s\n"),
			*RowName, Coord.X, Coord.Y, *UnitTypeID->ToString());
	}

	const FString FullPath = GetEnemyMapFilePath(FileName);
	const FString Directory = FPaths::GetPath(FullPath);
	if (!FPaths::DirectoryExists(Directory))
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	const bool bSuccess = FFileHelper::SaveStringToFile(CSVContent, *FullPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("MapEditor: saved %d enemy units to %s"), EnemyUnitTypeByCoord.Num(), *FullPath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MapEditor: failed to save enemy units to %s"), *FullPath);
	}
	return bSuccess;
}

bool ULFPMapEditorComponent::LoadEnemyUnits(const FString& FileName)
{
	EnemyUnitTypeByCoord.Empty();
	ClearEnemyUnitPreviews();

	const FString FullPath = GetEnemyMapFilePath(FileName);
	if (!FPaths::FileExists(FullPath))
	{
		UE_LOG(LogTemp, Log, TEXT("MapEditor: enemy unit file not found %s"), *FullPath);
		return true;
	}

	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *FullPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("MapEditor: failed to load enemy unit file %s"), *FullPath);
		return false;
	}

	UDataTable* TempTable = NewObject<UDataTable>(GetTransientPackage());
	TempTable->RowStruct = FLFPEnemyMapUnitRow::StaticStruct();

	TArray<FString> Problems = TempTable->CreateTableFromCSVString(CSVContent);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapEditor enemy CSV parse issue: %s"), *Problem);
	}

	TArray<FLFPEnemyMapUnitRow*> Rows;
	TempTable->GetAllRows<FLFPEnemyMapUnitRow>(TEXT("LoadEnemyUnits"), Rows);
	for (const FLFPEnemyMapUnitRow* Row : Rows)
	{
		if (!Row || Row->UnitTypeID.IsNone())
		{
			continue;
		}

		EnemyUnitTypeByCoord.Add(FIntPoint(Row->Q, Row->R), Row->UnitTypeID);
	}

	RebuildEnemyUnitPreviews();
	return true;
}

void ULFPMapEditorComponent::ClearEnemyUnitPreviews()
{
	for (auto& Pair : EnemyUnitPreviewByCoord)
	{
		if (ALFPTacticsUnit* PreviewUnit = Pair.Value.Get())
		{
			PreviewUnit->Destroy();
		}
	}
	EnemyUnitPreviewByCoord.Empty();
}

void ULFPMapEditorComponent::RebuildEnemyUnitPreviews()
{
	ClearEnemyUnitPreviews();

	ALFPHexGridManager* GM = GetGridManager();
	if (!GM)
	{
		return;
	}

	for (const auto& Pair : EnemyUnitTypeByCoord)
	{
		ALFPHexTile* Tile = GM->GetTileAtCoordinates(FLFPHexCoordinates(Pair.Key.X, Pair.Key.Y));
		if (!Tile)
		{
			UE_LOG(LogTemp, Warning, TEXT("MapEditor: enemy unit %s references missing tile (%d,%d)"),
				*Pair.Value.ToString(), Pair.Key.X, Pair.Key.Y);
			continue;
		}

		if (ALFPTacticsUnit* PreviewUnit = SpawnEnemyUnitPreview(Tile, Pair.Value))
		{
			EnemyUnitPreviewByCoord.Add(Pair.Key, PreviewUnit);
		}
	}
}

void ULFPMapEditorComponent::PlaceEnemyUnitAtTile(ALFPHexTile* Tile, FName UnitTypeID)
{
	if (!Tile || UnitTypeID.IsNone())
	{
		return;
	}

	const FLFPHexCoordinates Coord = Tile->GetCoordinates();
	const FIntPoint Key(Coord.Q, Coord.R);

	if (TObjectPtr<ALFPTacticsUnit>* ExistingPreview = EnemyUnitPreviewByCoord.Find(Key))
	{
		if (ALFPTacticsUnit* PreviewUnit = ExistingPreview->Get())
		{
			PreviewUnit->Destroy();
		}
		EnemyUnitPreviewByCoord.Remove(Key);
	}

	EnemyUnitTypeByCoord.Add(Key, UnitTypeID);
	if (ALFPTacticsUnit* PreviewUnit = SpawnEnemyUnitPreview(Tile, UnitTypeID))
	{
		EnemyUnitPreviewByCoord.Add(Key, PreviewUnit);
	}
}

void ULFPMapEditorComponent::RemoveEnemyUnitAtTile(ALFPHexTile* Tile)
{
	if (!Tile)
	{
		return;
	}

	const FLFPHexCoordinates Coord = Tile->GetCoordinates();
	const FIntPoint Key(Coord.Q, Coord.R);

	EnemyUnitTypeByCoord.Remove(Key);
	if (TObjectPtr<ALFPTacticsUnit>* Preview = EnemyUnitPreviewByCoord.Find(Key))
	{
		if (ALFPTacticsUnit* PreviewUnit = Preview->Get())
		{
			PreviewUnit->Destroy();
		}
		EnemyUnitPreviewByCoord.Remove(Key);
	}
}

ALFPTacticsUnit* ULFPMapEditorComponent::SpawnEnemyUnitPreview(ALFPHexTile* Tile, FName UnitTypeID)
{
	if (!Tile || UnitTypeID.IsNone() || !GetWorld())
	{
		return nullptr;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->UnitRegistry)
	{
		return nullptr;
	}

	TSubclassOf<ALFPTacticsUnit> UnitClass = GI->UnitRegistry->GetUnitClass(UnitTypeID);
	if (!UnitClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MapEditor: invalid enemy UnitTypeID %s"), *UnitTypeID.ToString());
		return nullptr;
	}

	const FLFPHexCoordinates Coord = Tile->GetCoordinates();
	const FTransform SpawnTransform(FRotator::ZeroRotator, Tile->GetActorLocation() + FVector(0.f, 0.f, 1.f));
	ALFPTacticsUnit* PreviewUnit = GetWorld()->SpawnActorDeferred<ALFPTacticsUnit>(
		UnitClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!PreviewUnit)
	{
		return nullptr;
	}

	PreviewUnit->bIsMapEditorPreviewUnit = true;
	PreviewUnit->Affiliation = EUnitAffiliation::UA_Enemy;
	PreviewUnit->UnitTypeID = UnitTypeID;
	PreviewUnit->SetStartCoordinates(Coord.Q, Coord.R);
	PreviewUnit->FinishSpawning(SpawnTransform);
	PreviewUnit->SetActorEnableCollision(false);
	PreviewUnit->SetCurrentCoordinates(Coord, false);
	return PreviewUnit;
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
			// 更新该格子及邻居的地形过渡
			FLFPHexCoordinates Coord = Tile->GetCoordinates();
			GM->UpdateTransitionsAround(Coord.Q, Coord.R);
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

	case ELFPMapEditorTool::MET_EnemyUnit:
		PlaceEnemyUnitAtTile(Tile, BrushEnemyUnitTypeID);
		break;

	case ELFPMapEditorTool::MET_RemoveEnemyUnit:
		RemoveEnemyUnitAtTile(Tile);
		break;

	case ELFPMapEditorTool::MET_RemoveTile:
		{
			FLFPHexCoordinates Coord = Tile->GetCoordinates();
			int32 RemovedQ = Coord.Q;
			int32 RemovedR = Coord.R;
			RemoveEnemyUnitAtTile(Tile);
			GM->RemoveTile(RemovedQ, RemovedR);
			// 更新被移除格子周围邻居的过渡
			GM->UpdateTransitionsAround(RemovedQ, RemovedR);
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
		// 更新新格子及邻居的地形过渡
		GM->UpdateTransitionsAround(Q, R);
	}
}

bool ULFPMapEditorComponent::SaveMap(const FString& FileName)
{
	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return false;

	FString FullPath = GetMapSaveDirectory() / FileName + TEXT(".csv");
	const bool bMapSaved = GM->SaveMapToCSV(FullPath);
	const bool bEnemiesSaved = SaveEnemyUnits(FileName);
	return bMapSaved && bEnemiesSaved;
}

bool ULFPMapEditorComponent::LoadMap(const FString& FileName)
{
	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return false;

	FString FullPath = GetMapSaveDirectory() / FileName + TEXT(".csv");
	const bool bMapLoaded = GM->LoadMapFromCSV(FullPath);
	if (bMapLoaded)
	{
		LoadEnemyUnits(FileName);
	}
	return bMapLoaded;
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
	MapFiles.RemoveAll([](const FString& File)
	{
		return File.EndsWith(TEXT("_enemies"));
	});
	return MapFiles;
}

void ULFPMapEditorComponent::NewMap(int32 Width, int32 Height)
{
	ALFPHexGridManager* GM = GetGridManager();
	if (!GM) return;

	GM->GenerateGrid(Width, Height);
	EnemyUnitTypeByCoord.Empty();
	ClearEnemyUnitPreviews();
	UE_LOG(LogTemp, Log, TEXT("新建地图: %dx%d"), Width, Height);
}
