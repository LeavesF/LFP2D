#include "LFP2D/WorldMap/LFPWorldMapManager.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "LFP2D/WorldMap/LFPWorldMapEdge.h"
#include "LFP2D/WorldMap/LFPWorldNodeDataAsset.h"
#include "LFP2D/WorldMap/LFPWorldMapPlayerState.h"
#include "LFP2D/WorldMap/LFPWorldMapPawn.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

ALFPWorldMapManager::ALFPWorldMapManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALFPWorldMapManager::BeginPlay()
{
	Super::BeginPlay();
}

// ============== 节点管理 ==============

ALFPWorldMapNode* ALFPWorldMapManager::SpawnNode(int32 NodeID, FVector2D Position, ELFPWorldNodeType NodeType)
{
	// 检查 ID 冲突
	if (NodeMap.Contains(NodeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnNode: 节点 ID %d 已存在"), NodeID);
		return NodeMap[NodeID];
	}

	if (!NodeActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnNode: NodeActorClass 未配置"));
		return nullptr;
	}

	FVector SpawnLocation(Position.X, Position.Y, 0.f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ALFPWorldMapNode* NewNode = GetWorld()->SpawnActor<ALFPWorldMapNode>(
		NodeActorClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (!NewNode) return nullptr;

	NewNode->NodeID = NodeID;
	NewNode->NodeType = NodeType;
	NewNode->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

	// 应用视觉数据
	if (ULFPWorldNodeDataAsset* VisualData = GetNodeVisualForType(NodeType))
	{
		NewNode->SetNodeVisualData(VisualData);
	}

	NodeMap.Add(NodeID, NewNode);

	// 确保邻接表有条目
	if (!AdjacencyList.Contains(NodeID))
	{
		AdjacencyList.Add(NodeID, TSet<int32>());
	}

	// 更新 ID 计数器
	if (NodeID >= NextNodeID)
	{
		NextNodeID = NodeID + 1;
	}

	return NewNode;
}

bool ALFPWorldMapManager::RemoveNode(int32 NodeID)
{
	ALFPWorldMapNode** Found = NodeMap.Find(NodeID);
	if (!Found) return false;

	// 先移除所有关联边
	if (TArray<FIntPoint>* EdgeKeys = NodeEdgeIndex.Find(NodeID))
	{
		// 复制一份，因为 RemoveEdge 会修改 NodeEdgeIndex
		TArray<FIntPoint> KeysCopy = *EdgeKeys;
		for (const FIntPoint& Key : KeysCopy)
		{
			if (ALFPWorldMapEdge** EdgeFound = EdgeMap.Find(Key))
			{
				int32 OtherID = (Key.X == NodeID) ? Key.Y : Key.X;
				RemoveEdge(NodeID, OtherID);
			}
		}
	}

	// 销毁 Actor
	if (*Found)
	{
		(*Found)->Destroy();
	}

	NodeMap.Remove(NodeID);
	AdjacencyList.Remove(NodeID);
	NodeEdgeIndex.Remove(NodeID);

	return true;
}

ALFPWorldMapNode* ALFPWorldMapManager::GetNode(int32 NodeID) const
{
	if (const auto* Found = NodeMap.Find(NodeID))
	{
		return *Found;
	}
	return nullptr;
}

TArray<ALFPWorldMapNode*> ALFPWorldMapManager::GetNeighbors(int32 NodeID) const
{
	TArray<ALFPWorldMapNode*> Result;
	if (const TSet<int32>* Neighbors = AdjacencyList.Find(NodeID))
	{
		for (int32 NeighborID : *Neighbors)
		{
			if (ALFPWorldMapNode* Node = GetNode(NeighborID))
			{
				Result.Add(Node);
			}
		}
	}
	return Result;
}

int32 ALFPWorldMapManager::GetNextNodeID() const
{
	return NextNodeID;
}

// ============== 边管理 ==============

ALFPWorldMapEdge* ALFPWorldMapManager::AddEdge(int32 FromID, int32 ToID, int32 TurnCost)
{
	// 验证节点存在
	if (!NodeMap.Contains(FromID) || !NodeMap.Contains(ToID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AddEdge: 节点 %d 或 %d 不存在"), FromID, ToID);
		return nullptr;
	}

	// 不能自连
	if (FromID == ToID)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddEdge: 不能自连节点 %d"), FromID);
		return nullptr;
	}

	FIntPoint Key = MakeEdgeKey(FromID, ToID);

	// 检查重复
	if (EdgeMap.Contains(Key))
	{
		UE_LOG(LogTemp, Warning, TEXT("AddEdge: 边 %d-%d 已存在"), FromID, ToID);
		return EdgeMap[Key];
	}

	if (!EdgeActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("AddEdge: EdgeActorClass 未配置"));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	ALFPWorldMapEdge* NewEdge = GetWorld()->SpawnActor<ALFPWorldMapEdge>(
		EdgeActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (!NewEdge) return nullptr;

	NewEdge->FromNodeID = FromID;
	NewEdge->ToNodeID = ToID;
	NewEdge->TravelTurnCost = TurnCost;
	NewEdge->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

	// 设置边精灵
	if (EdgeSprite)
	{
		NewEdge->SetEdgeSprite(EdgeSprite);
	}

	// 更新视觉位置
	ALFPWorldMapNode* FromNode = NodeMap[FromID];
	ALFPWorldMapNode* ToNode = NodeMap[ToID];
	NewEdge->UpdateVisualPosition(FromNode->GetActorLocation(), ToNode->GetActorLocation());

	// 存储
	EdgeMap.Add(Key, NewEdge);

	// 更新邻接表（双向）
	AdjacencyList.FindOrAdd(FromID).Add(ToID);
	AdjacencyList.FindOrAdd(ToID).Add(FromID);

	// 更新节点-边索引
	NodeEdgeIndex.FindOrAdd(FromID).Add(Key);
	NodeEdgeIndex.FindOrAdd(ToID).Add(Key);

	return NewEdge;
}

bool ALFPWorldMapManager::RemoveEdge(int32 FromID, int32 ToID)
{
	FIntPoint Key = MakeEdgeKey(FromID, ToID);

	ALFPWorldMapEdge** Found = EdgeMap.Find(Key);
	if (!Found) return false;

	// 销毁 Actor
	if (*Found)
	{
		(*Found)->Destroy();
	}

	EdgeMap.Remove(Key);

	// 更新邻接表
	if (TSet<int32>* FromNeighbors = AdjacencyList.Find(FromID))
	{
		FromNeighbors->Remove(ToID);
	}
	if (TSet<int32>* ToNeighbors = AdjacencyList.Find(ToID))
	{
		ToNeighbors->Remove(FromID);
	}

	// 更新节点-边索引
	if (TArray<FIntPoint>* FromEdges = NodeEdgeIndex.Find(FromID))
	{
		FromEdges->Remove(Key);
	}
	if (TArray<FIntPoint>* ToEdges = NodeEdgeIndex.Find(ToID))
	{
		ToEdges->Remove(Key);
	}

	return true;
}

ALFPWorldMapEdge* ALFPWorldMapManager::GetEdge(int32 FromID, int32 ToID) const
{
	FIntPoint Key = MakeEdgeKey(FromID, ToID);
	if (const auto* Found = EdgeMap.Find(Key))
	{
		return *Found;
	}
	return nullptr;
}

// ============== 地图操作 ==============

void ALFPWorldMapManager::ClearMap()
{
	// 先销毁边
	for (auto& Pair : EdgeMap)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	EdgeMap.Empty();

	// 再销毁节点
	for (auto& Pair : NodeMap)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	NodeMap.Empty();

	AdjacencyList.Empty();
	NodeEdgeIndex.Empty();
	NextNodeID = 0;
}

// ============== CSV 保存/加载 ==============

FString ALFPWorldMapManager::GetWorldMapSaveDirectory() const
{
	return FPaths::ProjectSavedDir() / TEXT("WorldMaps");
}

FIntPoint ALFPWorldMapManager::MakeEdgeKey(int32 A, int32 B)
{
	return FIntPoint(FMath::Min(A, B), FMath::Max(A, B));
}

bool ALFPWorldMapManager::SaveWorldMap(const FString& MapName)
{
	FString Dir = GetWorldMapSaveDirectory();
	bool bNodesOk = SaveNodesToCSV(Dir / (MapName + TEXT("_nodes.csv")));
	bool bEdgesOk = SaveEdgesToCSV(Dir / (MapName + TEXT("_edges.csv")));
	return bNodesOk && bEdgesOk;
}

bool ALFPWorldMapManager::LoadWorldMap(const FString& MapName)
{
	ClearMap();

	FString Dir = GetWorldMapSaveDirectory();
	bool bNodesOk = LoadNodesFromCSV(Dir / (MapName + TEXT("_nodes.csv")));
	if (!bNodesOk) return false;

	bool bEdgesOk = LoadEdgesFromCSV(Dir / (MapName + TEXT("_edges.csv")));

	if (bEdgesOk)
	{
		CurrentWorldMapName = MapName;
	}

	return bEdgesOk;
}

TArray<FString> ALFPWorldMapManager::GetSavedWorldMapList()
{
	TArray<FString> MapNames;
	FString Dir = GetWorldMapSaveDirectory();

	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *(Dir / TEXT("*_nodes.csv")), true, false);

	for (const FString& File : FoundFiles)
	{
		// 去掉 "_nodes.csv" 后缀
		FString Name = File;
		Name.RemoveFromEnd(TEXT("_nodes.csv"));
		MapNames.Add(Name);
	}

	return MapNames;
}

bool ALFPWorldMapManager::SaveNodesToCSV(const FString& FilePath)
{
	// CSV 头部（UE DataTable 格式）
	FString CSVContent = TEXT("---,NodeID,PosX,PosY,NodeType,BattleMapName,StarRating,bCanEscape,BaseGoldReward,BaseFoodReward,EventID,TownBuildingList,ShopID,HireMarketID,PrerequisiteNodeIDs\n");

	for (const auto& Pair : NodeMap)
	{
		ALFPWorldMapNode* Node = Pair.Value;
		if (!Node) continue;

		FLFPWorldNodeRow Row = Node->ExportToRowData();
		FString RowName = FString::FromInt(Row.NodeID);

		// 节点类型枚举名
		FString NodeTypeStr = UEnum::GetValueAsString(Row.NodeType);
		NodeTypeStr = NodeTypeStr.RightChop(NodeTypeStr.Find(TEXT("::")) + 2);

		CSVContent += FString::Printf(TEXT("%s,%d,%f,%f,%s,%s,%d,%s,%d,%d,%s,%s,%s,%s,%s\n"),
			*RowName,
			Row.NodeID,
			Row.PosX,
			Row.PosY,
			*NodeTypeStr,
			*Row.BattleMapName,
			Row.StarRating,
			Row.bCanEscape ? TEXT("True") : TEXT("False"),
			Row.BaseGoldReward,
			Row.BaseFoodReward,
			*Row.EventID,
			*Row.TownBuildingList,
			*Row.ShopID.ToString(),
			*Row.HireMarketID.ToString(),
			*Row.PrerequisiteNodeIDs);
	}

	// 确保目录存在
	FString Directory = FPaths::GetPath(FilePath);
	if (!FPaths::DirectoryExists(Directory))
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	bool bSuccess = FFileHelper::SaveStringToFile(CSVContent, *FilePath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("SaveNodesToCSV: 保存 %d 个节点到 %s"), NodeMap.Num(), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SaveNodesToCSV: 保存失败 %s"), *FilePath);
	}
	return bSuccess;
}

bool ALFPWorldMapManager::SaveEdgesToCSV(const FString& FilePath)
{
	FString CSVContent = TEXT("---,FromNodeID,ToNodeID,TravelTurnCost\n");

	for (const auto& Pair : EdgeMap)
	{
		ALFPWorldMapEdge* Edge = Pair.Value;
		if (!Edge) continue;

		FLFPWorldEdgeRow Row = Edge->ExportToRowData();
		FString RowName = FString::Printf(TEXT("%d_%d"), Row.FromNodeID, Row.ToNodeID);

		CSVContent += FString::Printf(TEXT("%s,%d,%d,%d\n"),
			*RowName, Row.FromNodeID, Row.ToNodeID, Row.TravelTurnCost);
	}

	FString Directory = FPaths::GetPath(FilePath);
	if (!FPaths::DirectoryExists(Directory))
	{
		IFileManager::Get().MakeDirectory(*Directory, true);
	}

	bool bSuccess = FFileHelper::SaveStringToFile(CSVContent, *FilePath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("SaveEdgesToCSV: 保存 %d 条边到 %s"), EdgeMap.Num(), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SaveEdgesToCSV: 保存失败 %s"), *FilePath);
	}
	return bSuccess;
}

bool ALFPWorldMapManager::LoadNodesFromCSV(const FString& FilePath)
{
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadNodesFromCSV: 无法加载文件 %s"), *FilePath);
		return false;
	}

	// 使用临时 DataTable 解析 CSV
	UDataTable* TempTable = NewObject<UDataTable>(GetTransientPackage());
	TempTable->RowStruct = FLFPWorldNodeRow::StaticStruct();

	TArray<FString> Problems = TempTable->CreateTableFromCSVString(CSVContent);
	if (Problems.Num() > 0)
	{
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogTemp, Warning, TEXT("节点 CSV 解析问题: %s"), *Problem);
		}
	}

	TArray<FLFPWorldNodeRow*> AllRows;
	TempTable->GetAllRows<FLFPWorldNodeRow>(TEXT("LoadNodes"), AllRows);

	for (const FLFPWorldNodeRow* Row : AllRows)
	{
		if (!Row) continue;

		ALFPWorldMapNode* Node = SpawnNode(Row->NodeID, FVector2D(Row->PosX, Row->PosY), Row->NodeType);
		if (Node)
		{
			Node->InitFromRowData(*Row);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("LoadNodesFromCSV: 加载了 %d 个节点"), AllRows.Num());
	return true;
}

bool ALFPWorldMapManager::LoadEdgesFromCSV(const FString& FilePath)
{
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadEdgesFromCSV: 无法加载文件 %s"), *FilePath);
		return false;
	}

	UDataTable* TempTable = NewObject<UDataTable>(GetTransientPackage());
	TempTable->RowStruct = FLFPWorldEdgeRow::StaticStruct();

	TArray<FString> Problems = TempTable->CreateTableFromCSVString(CSVContent);
	if (Problems.Num() > 0)
	{
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogTemp, Warning, TEXT("边 CSV 解析问题: %s"), *Problem);
		}
	}

	TArray<FLFPWorldEdgeRow*> AllRows;
	TempTable->GetAllRows<FLFPWorldEdgeRow>(TEXT("LoadEdges"), AllRows);

	for (const FLFPWorldEdgeRow* Row : AllRows)
	{
		if (!Row) continue;
		AddEdge(Row->FromNodeID, Row->ToNodeID, Row->TravelTurnCost);
	}

	UE_LOG(LogTemp, Log, TEXT("LoadEdgesFromCSV: 加载了 %d 条边"), AllRows.Num());
	return true;
}

// ============== 导出数据 ==============

TArray<FLFPWorldNodeRow> ALFPWorldMapManager::ExportNodeData() const
{
	TArray<FLFPWorldNodeRow> Result;
	for (const auto& Pair : NodeMap)
	{
		if (Pair.Value)
		{
			Result.Add(Pair.Value->ExportToRowData());
		}
	}
	return Result;
}

TArray<FLFPWorldEdgeRow> ALFPWorldMapManager::ExportEdgeData() const
{
	TArray<FLFPWorldEdgeRow> Result;
	for (const auto& Pair : EdgeMap)
	{
		if (Pair.Value)
		{
			Result.Add(Pair.Value->ExportToRowData());
		}
	}
	return Result;
}

// ============== 视觉注册表 ==============

ULFPWorldNodeDataAsset* ALFPWorldMapManager::GetNodeVisualForType(ELFPWorldNodeType NodeType) const
{
	if (const TObjectPtr<ULFPWorldNodeDataAsset>* Found = NodeVisualRegistry.Find(NodeType))
	{
		return *Found;
	}
	return nullptr;
}

// ============== 迷雾系统 ==============

TSet<int32> ALFPWorldMapManager::GetNodesWithinGraphDistance(int32 StartNodeID, int32 MaxDistance) const
{
	TSet<int32> Result;

	if (!AdjacencyList.Contains(StartNodeID)) return Result;

	// BFS
	TQueue<TPair<int32, int32>> Queue; // <NodeID, Distance>
	TSet<int32> Visited;

	Queue.Enqueue(TPair<int32, int32>(StartNodeID, 0));
	Visited.Add(StartNodeID);

	while (!Queue.IsEmpty())
	{
		TPair<int32, int32> Current;
		Queue.Dequeue(Current);

		int32 CurrentID = Current.Key;
		int32 CurrentDist = Current.Value;

		Result.Add(CurrentID);

		if (CurrentDist >= MaxDistance) continue;

		if (const TSet<int32>* Neighbors = AdjacencyList.Find(CurrentID))
		{
			for (int32 NeighborID : *Neighbors)
			{
				if (!Visited.Contains(NeighborID))
				{
					Visited.Add(NeighborID);
					Queue.Enqueue(TPair<int32, int32>(NeighborID, CurrentDist + 1));
				}
			}
		}
	}

	return Result;
}

void ALFPWorldMapManager::UpdateFog()
{
	if (!PlayerState) return;

	// 获取视野范围内的节点
	TSet<int32> VisibleNodes = GetNodesWithinGraphDistance(PlayerState->CurrentNodeID, FogVisionRange);

	// 添加到永久揭露集合
	for (int32 NodeID : VisibleNodes)
	{
		PlayerState->RevealedNodeIDs.Add(NodeID);
	}

	// 更新所有节点的视觉状态
	for (auto& Pair : NodeMap)
	{
		if (!Pair.Value) continue;

		Pair.Value->bIsRevealed = PlayerState->RevealedNodeIDs.Contains(Pair.Key);
		Pair.Value->UpdateVisualState();
	}

	// 更新边的可见性（两端都揭露才可见）
	for (auto& Pair : EdgeMap)
	{
		if (!Pair.Value) continue;

		bool bBothRevealed = PlayerState->RevealedNodeIDs.Contains(Pair.Key.X)
			&& PlayerState->RevealedNodeIDs.Contains(Pair.Key.Y);
		Pair.Value->SetActorHiddenInGame(!bBothRevealed);
	}
}

// ============== 玩家状态 ==============

void ALFPWorldMapManager::InitializePlayer(int32 StartNodeID)
{
	if (!PlayerState)
	{
		PlayerState = NewObject<ULFPWorldMapPlayerState>(this);
	}

	PlayerState->Initialize(StartNodeID);

	// 生成或移动玩家棋子到起始节点
	ALFPWorldMapNode* StartNode = GetNode(StartNodeID);
	FVector StartLocation = StartNode ? StartNode->GetActorLocation() + FVector(0, 0, 1) : FVector::ZeroVector;

	if (!PlayerPawn && PawnActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		PlayerPawn = GetWorld()->SpawnActor<ALFPWorldMapPawn>(
			PawnActorClass, StartLocation, FRotator::ZeroRotator, SpawnParams);

		if (PlayerPawn)
		{
			PlayerPawn->OnMoveComplete.AddDynamic(this, &ALFPWorldMapManager::OnPawnMoveComplete);
		}
	}
	else if (PlayerPawn)
	{
		PlayerPawn->SetLocationImmediate(StartLocation);
	}

	// 初始迷雾更新
	UpdateFog();

	UE_LOG(LogTemp, Log, TEXT("玩家初始化在节点 %d"), StartNodeID);
}

bool ALFPWorldMapManager::MovePlayer(int32 TargetNodeID)
{
	if (!PlayerState) return false;

	// 棋子移动中不允许再次移动
	if (PlayerPawn && PlayerPawn->IsMoving()) return false;

	bool bMoved = PlayerState->MoveToNode(TargetNodeID, this);

	if (bMoved)
	{
		ALFPWorldMapNode* TargetNode = GetNode(TargetNodeID);
		if (!TargetNode) return true;

		// 如果有棋子，启动移动动画，延迟迷雾更新和事件触发
		if (PlayerPawn)
		{
			PendingMoveTargetNodeID = TargetNodeID;
			FVector TargetLocation = TargetNode->GetActorLocation() + FVector(0, 0, 1);
			PlayerPawn->MoveToLocation(TargetLocation);
		}
		else
		{
			// 没有棋子，直接执行后续逻辑
			HandlePostMove(TargetNodeID);
		}
	}

	return bMoved;
}

void ALFPWorldMapManager::OnPawnMoveComplete()
{
	if (PendingMoveTargetNodeID >= 0)
	{
		int32 NodeID = PendingMoveTargetNodeID;
		PendingMoveTargetNodeID = -1;
		HandlePostMove(NodeID);
	}
}

void ALFPWorldMapManager::HandlePostMove(int32 TargetNodeID)
{
	// 更新迷雾
	UpdateFog();

	// 触发节点事件（首次进入未触发的节点）
	ALFPWorldMapNode* TargetNode = GetNode(TargetNodeID);
	if (TargetNode && !TargetNode->bHasBeenTriggered)
	{
		switch (TargetNode->NodeType)
		{
		case ELFPWorldNodeType::WNT_Battle:
		case ELFPWorldNodeType::WNT_Event:
		case ELFPWorldNodeType::WNT_Boss:
			TargetNode->bHasBeenTriggered = true;
			TargetNode->UpdateVisualState();
			break;
		default:
			break;
		}
	}

	// 回合压力检查
	if (PlayerState && PlayerState->IsPastPressureThreshold())
	{
		UE_LOG(LogTemp, Warning, TEXT("回合压力: 已超过 %d 回合阈值！当前 %d 回合"),
			PlayerState->TurnPressureThreshold, PlayerState->CurrentTurn);
	}
}

bool ALFPWorldMapManager::IsPawnMoving() const
{
	return PlayerPawn && PlayerPawn->IsMoving();
}

void ALFPWorldMapManager::RestoreTriggeredNodes(const TSet<int32>& TriggeredNodeIDs)
{
	for (int32 NodeID : TriggeredNodeIDs)
	{
		if (ALFPWorldMapNode* Node = GetNode(NodeID))
		{
			Node->bHasBeenTriggered = true;
			Node->UpdateVisualState();
		}
	}
}
