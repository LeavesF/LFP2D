#include "LFP2D/WorldMap/LFPWorldMapEditorComponent.h"
#include "LFP2D/WorldMap/LFPWorldMapManager.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "LFP2D/WorldMap/LFPWorldMapEdge.h"
#include "LFP2D/HexGrid/LFPMapEditorComponent.h"
#include "Kismet/GameplayStatics.h"

ULFPWorldMapEditorComponent::ULFPWorldMapEditorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULFPWorldMapEditorComponent::BeginPlay()
{
	Super::BeginPlay();
}

ALFPWorldMapManager* ULFPWorldMapEditorComponent::GetWorldMapManager() const
{
	if (!CachedWorldMapManager)
	{
		TArray<AActor*> Managers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPWorldMapManager::StaticClass(), Managers);
		if (Managers.Num() > 0)
		{
			CachedWorldMapManager = Cast<ALFPWorldMapManager>(Managers[0]);
		}
	}
	return CachedWorldMapManager;
}

void ULFPWorldMapEditorComponent::ToggleEditorMode()
{
	bEditorActive = !bEditorActive;

	ALFPWorldMapManager* Manager = GetWorldMapManager();

	if (bEditorActive)
	{
		// 编辑器开启：显示所有节点（临时揭开迷雾）
		if (Manager)
		{
			for (const auto& Pair : Manager->GetNodeMap())
			{
				if (ALFPWorldMapNode* Node = Pair.Value)
				{
					Node->bIsRevealed = true;
					Node->UpdateVisualState();
				}
			}

			// 显示所有边
			for (const auto& Pair : Manager->GetEdgeMap())
			{
				if (ALFPWorldMapEdge* Edge = Pair.Value)
				{
					Edge->SetActorHiddenInGame(false);
				}
			}
		}
	}
	else
	{
		// 编辑器关闭：恢复迷雾状态
		CurrentTool = ELFPWorldMapEditorTool::WMET_None;
		SelectedNode = nullptr;
		EdgeStartNode = nullptr;
		EdgeRemovalStartNode = nullptr;

		if (Manager)
		{
			Manager->UpdateFog();
		}
	}

	OnEditorModeChanged.Broadcast(bEditorActive);
	UE_LOG(LogTemp, Log, TEXT("世界地图编辑器: %s"), bEditorActive ? TEXT("开启") : TEXT("关闭"));
}

void ULFPWorldMapEditorComponent::SetCurrentTool(ELFPWorldMapEditorTool Tool)
{
	CurrentTool = Tool;
	OnToolChanged.Broadcast(CurrentTool);

	// 切换工具时清除临时状态
	EdgeStartNode = nullptr;
	EdgeRemovalStartNode = nullptr;
}

ALFPWorldMapNode* ULFPWorldMapEditorComponent::PlaceNodeAt(FVector2D WorldPos)
{
	if (!bEditorActive) return nullptr;

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return nullptr;

	int32 NewID = Manager->GetNextNodeID();
	ALFPWorldMapNode* NewNode = Manager->SpawnNode(NewID, WorldPos, BrushNodeType);

	if (NewNode)
	{
		// 编辑器模式下新节点直接可见
		NewNode->bIsRevealed = true;
		NewNode->UpdateVisualState();

		// 应用笔刷参数
		NewNode->BattleMapName = BrushBattleMapName;
		NewNode->StarRating = BrushStarRating;
		NewNode->bCanEscape = BrushCanEscape;
		NewNode->EventID = BrushEventID;
		NewNode->BaseGoldReward = BrushBaseGoldReward;
		NewNode->BaseFoodReward = BrushBaseFoodReward;
		NewNode->TownBuildingList = BrushTownBuildingList;
		NewNode->ShopID = BrushShopID;
		NewNode->HireMarketID = BrushHireMarketID;

		UE_LOG(LogTemp, Log, TEXT("放置节点 ID %d 在 (%.1f, %.1f)"), NewID, WorldPos.X, WorldPos.Y);
	}

	return NewNode;
}

void ULFPWorldMapEditorComponent::SelectNode(ALFPWorldMapNode* Node)
{
	// 取消之前选中节点的高亮
	if (SelectedNode)
	{
		SelectedNode->SetHighlighted(false);
	}

	SelectedNode = Node;

	// 高亮新选中的节点
	if (SelectedNode)
	{
		SelectedNode->SetHighlighted(true);
	}

	OnNodeSelected.Broadcast(SelectedNode);
}

void ULFPWorldMapEditorComponent::ApplyParamsToSelectedNode()
{
	if (!SelectedNode) return;

	SelectedNode->NodeType = BrushNodeType;
	SelectedNode->BattleMapName = BrushBattleMapName;
	SelectedNode->StarRating = BrushStarRating;
	SelectedNode->bCanEscape = BrushCanEscape;
	SelectedNode->EventID = BrushEventID;
	SelectedNode->BaseGoldReward = BrushBaseGoldReward;
	SelectedNode->BaseFoodReward = BrushBaseFoodReward;
	SelectedNode->TownBuildingList = BrushTownBuildingList;
	SelectedNode->ShopID = BrushShopID;
	SelectedNode->HireMarketID = BrushHireMarketID;

	// 更新视觉（节点类型可能变了）
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (Manager)
	{
		if (auto* VisualData = Manager->GetNodeVisualForType(BrushNodeType))
		{
			SelectedNode->SetNodeVisualData(VisualData);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("应用参数到节点 ID %d"), SelectedNode->NodeID);
}

void ULFPWorldMapEditorComponent::HandleEdgeConnection(ALFPWorldMapNode* Node)
{
	if (!Node) return;

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	if (!EdgeStartNode)
	{
		// 第一次点击：记录起点
		EdgeStartNode = Node;
		EdgeStartNode->SetHighlighted(true);
		UE_LOG(LogTemp, Log, TEXT("选择边起点: 节点 %d"), EdgeStartNode->NodeID);
	}
	else
	{
		// 第二次点击：创建边
		if (EdgeStartNode != Node)
		{
			Manager->AddEdge(EdgeStartNode->NodeID, Node->NodeID, BrushEdgeTurnCost);
			UE_LOG(LogTemp, Log, TEXT("创建边: %d -> %d (消耗 %d 回合)"),
				EdgeStartNode->NodeID, Node->NodeID, BrushEdgeTurnCost);
		}

		// 清除高亮和状态
		EdgeStartNode->SetHighlighted(false);
		EdgeStartNode = nullptr;
	}
}

void ULFPWorldMapEditorComponent::HandleEdgeRemoval(ALFPWorldMapNode* Node)
{
	if (!Node) return;

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	if (!EdgeRemovalStartNode)
	{
		// 第一次点击：记录起点
		EdgeRemovalStartNode = Node;
		EdgeRemovalStartNode->SetHighlighted(true);
		UE_LOG(LogTemp, Log, TEXT("选择要删除的边起点: 节点 %d"), EdgeRemovalStartNode->NodeID);
	}
	else
	{
		// 第二次点击：删除边
		if (EdgeRemovalStartNode != Node)
		{
			bool bRemoved = Manager->RemoveEdge(EdgeRemovalStartNode->NodeID, Node->NodeID);
			if (bRemoved)
			{
				UE_LOG(LogTemp, Log, TEXT("删除边: %d <-> %d"), EdgeRemovalStartNode->NodeID, Node->NodeID);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("边不存在: %d <-> %d"), EdgeRemovalStartNode->NodeID, Node->NodeID);
			}
		}

		// 清除高亮和状态
		EdgeRemovalStartNode->SetHighlighted(false);
		EdgeRemovalStartNode = nullptr;
	}
}

void ULFPWorldMapEditorComponent::MoveSelectedNodeTo(FVector2D NewPos)
{
	if (!SelectedNode) return;

	SelectedNode->SetPosition2D(NewPos);

	// 更新所有关联边的视觉位置
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	TArray<ALFPWorldMapNode*> Neighbors = Manager->GetNeighbors(SelectedNode->NodeID);
	for (ALFPWorldMapNode* Neighbor : Neighbors)
	{
		if (ALFPWorldMapEdge* Edge = Manager->GetEdge(SelectedNode->NodeID, Neighbor->NodeID))
		{
			Edge->UpdateVisualPosition(SelectedNode->GetActorLocation(), Neighbor->GetActorLocation());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("移动节点 %d 到 (%.1f, %.1f)"), SelectedNode->NodeID, NewPos.X, NewPos.Y);
}

void ULFPWorldMapEditorComponent::RemoveSelectedNode()
{
	if (!SelectedNode) return;

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	int32 NodeID = SelectedNode->NodeID;
	Manager->RemoveNode(NodeID);
	SelectedNode = nullptr;

	UE_LOG(LogTemp, Log, TEXT("删除节点 %d"), NodeID);
}

void ULFPWorldMapEditorComponent::EnterBattleMapEditor()
{
	if (!SelectedNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnterBattleMapEditor: 未选中节点"));
		return;
	}

	if (SelectedNode->NodeType != ELFPWorldNodeType::WNT_Battle)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnterBattleMapEditor: 选中的不是战斗节点"));
		return;
	}

	if (SelectedNode->BattleMapName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnterBattleMapEditor: 战斗地图名称为空"));
		return;
	}

	// 获取战斗地图编辑器组件（在同一个 PlayerController 上）
	ULFPMapEditorComponent* BattleEditor = GetOwner()->FindComponentByClass<ULFPMapEditorComponent>();
	if (!BattleEditor)
	{
		UE_LOG(LogTemp, Error, TEXT("EnterBattleMapEditor: 找不到战斗地图编辑器组件"));
		return;
	}

	// 加载战斗地图
	bool bLoaded = BattleEditor->LoadMap(SelectedNode->BattleMapName);
	if (!bLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnterBattleMapEditor: 加载战斗地图失败，创建新地图"));
		BattleEditor->NewMap(10, 10);
	}

	// 激活战斗地图编辑器
	if (!BattleEditor->IsEditorActive())
	{
		BattleEditor->ToggleEditorMode();
	}

	bIsEditingBattleMap = true;
	UE_LOG(LogTemp, Log, TEXT("进入战斗地图编辑: %s"), *SelectedNode->BattleMapName);
}

void ULFPWorldMapEditorComponent::ReturnFromBattleMapEditor()
{
	if (!bIsEditingBattleMap) return;

	ULFPMapEditorComponent* BattleEditor = GetOwner()->FindComponentByClass<ULFPMapEditorComponent>();
	if (BattleEditor && SelectedNode)
	{
		// 保存战斗地图（自动关联回节点）
		BattleEditor->SaveMap(SelectedNode->BattleMapName);

		// 关闭战斗地图编辑器
		if (BattleEditor->IsEditorActive())
		{
			BattleEditor->ToggleEditorMode();
		}
	}

	bIsEditingBattleMap = false;
	UE_LOG(LogTemp, Log, TEXT("返回世界地图编辑器"));
}

bool ULFPWorldMapEditorComponent::SaveWorldMap(const FString& MapName)
{
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return false;

	return Manager->SaveWorldMap(MapName);
}

bool ULFPWorldMapEditorComponent::LoadWorldMap(const FString& MapName)
{
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return false;

	return Manager->LoadWorldMap(MapName);
}

TArray<FString> ULFPWorldMapEditorComponent::GetSavedWorldMapList()
{
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return TArray<FString>();

	return Manager->GetSavedWorldMapList();
}

void ULFPWorldMapEditorComponent::NewWorldMap()
{
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	Manager->ClearMap();
	UE_LOG(LogTemp, Log, TEXT("新建空白世界地图"));
}
