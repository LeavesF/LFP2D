#include "LFP2D/WorldMap/LFPWorldMapPlayerState.h"
#include "LFP2D/WorldMap/LFPWorldMapManager.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "LFP2D/WorldMap/LFPWorldMapEdge.h"

void ULFPWorldMapPlayerState::Initialize(int32 StartNodeID)
{
	CurrentNodeID = StartNodeID;
	CurrentTurn = 0;
	VisitedNodeIDs.Empty();
	RevealedNodeIDs.Empty();

	VisitedNodeIDs.Add(StartNodeID);
}

bool ULFPWorldMapPlayerState::MoveToNode(int32 TargetNodeID, ALFPWorldMapManager* Manager)
{
	if (!Manager) return false;

	// 验证边存在
	ALFPWorldMapEdge* Edge = Manager->GetEdge(CurrentNodeID, TargetNodeID);
	if (!Edge)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToNode: 节点 %d 和 %d 之间没有边"), CurrentNodeID, TargetNodeID);
		return false;
	}

	// 验证目标节点存在
	ALFPWorldMapNode* TargetNode = Manager->GetNode(TargetNodeID);
	if (!TargetNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToNode: 目标节点 %d 不存在"), TargetNodeID);
		return false;
	}

	// 消耗回合
	int32 TurnCost = Edge->TravelTurnCost;
	CurrentTurn += TurnCost;

	// 更新位置
	int32 PrevNodeID = CurrentNodeID;
	CurrentNodeID = TargetNodeID;

	// 标记为已访问
	VisitedNodeIDs.Add(TargetNodeID);

	UE_LOG(LogTemp, Log, TEXT("移动: 节点 %d -> %d，消耗 %d 回合（总计 %d）"),
		PrevNodeID, TargetNodeID, TurnCost, CurrentTurn);

	// 广播
	OnTurnChanged.Broadcast(CurrentTurn, TurnPressureThreshold);
	OnPlayerNodeChanged.Broadcast(TargetNode);

	return true;
}

void ULFPWorldMapPlayerState::SetCurrentNodeDirectly(int32 TargetNodeID, ALFPWorldMapNode* TargetNode)
{
	CurrentNodeID = TargetNodeID;
	VisitedNodeIDs.Add(TargetNodeID);

	UE_LOG(LogTemp, Log, TEXT("传送: 直接移动到节点 %d"), TargetNodeID);

	if (TargetNode)
	{
		OnPlayerNodeChanged.Broadcast(TargetNode);
	}
}

TArray<int32> ULFPWorldMapPlayerState::GetReachableNodeIDs(ALFPWorldMapManager* Manager) const
{
	TArray<int32> Result;
	if (!Manager) return Result;

	TArray<ALFPWorldMapNode*> Neighbors = Manager->GetNeighbors(CurrentNodeID);
	for (ALFPWorldMapNode* Neighbor : Neighbors)
	{
		if (!Neighbor) continue;

		// 检查解锁条件（前置节点必须已访问）
		bool bUnlocked = true;
		if (!Neighbor->PrerequisiteNodeIDs.IsEmpty())
		{
			TArray<FString> PrereqIDs;
			Neighbor->PrerequisiteNodeIDs.ParseIntoArray(PrereqIDs, TEXT(";"), true);
			for (const FString& IDStr : PrereqIDs)
			{
				int32 PrereqID = FCString::Atoi(*IDStr);
				if (!VisitedNodeIDs.Contains(PrereqID))
				{
					bUnlocked = false;
					break;
				}
			}
		}

		if (bUnlocked)
		{
			Result.Add(Neighbor->NodeID);
		}
	}

	return Result;
}
