#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "LFP2D/WorldMap/LFPWorldNodeDataAsset.h"
#include "PaperSpriteComponent.h"

ALFPWorldMapNode::ALFPWorldMapNode()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootSceneComponent);

	NodeSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("NodeSprite"));
	NodeSpriteComponent->SetupAttachment(RootSceneComponent);
	// 精灵默认在 XZ 平面（竖直），旋转使其平躺在 XY 平面（俯视角）
	NodeSpriteComponent->SetRelativeRotation(FRotator(0.f, 90.f, -90.f));
	NodeSpriteComponent->TranslucencySortPriority = 1;

	HighlightSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("HighlightSprite"));
	HighlightSpriteComponent->SetupAttachment(RootSceneComponent);
	// 高亮层在节点精灵下方（Z 略低）
	HighlightSpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, -1.f));
	HighlightSpriteComponent->SetRelativeRotation(FRotator(0.f, 90.f, -90.f));
	HighlightSpriteComponent->TranslucencySortPriority = 2;
	HighlightSpriteComponent->SetVisibility(false);
}

void ALFPWorldMapNode::InitFromRowData(const FLFPWorldNodeRow& Row)
{
	NodeID = Row.NodeID;
	NodeType = Row.NodeType;
	BattleMapName = Row.BattleMapName;
	StarRating = Row.StarRating;
	bCanEscape = Row.bCanEscape;
	BaseGoldReward = Row.BaseGoldReward;
	BaseFoodReward = Row.BaseFoodReward;
	EventID = Row.EventID;
	PrerequisiteNodeIDs = Row.PrerequisiteNodeIDs;

	SetActorLocation(FVector(Row.PosX, Row.PosY, 0.f));
}

FLFPWorldNodeRow ALFPWorldMapNode::ExportToRowData() const
{
	FLFPWorldNodeRow Row;
	Row.NodeID = NodeID;
	FVector Loc = GetActorLocation();
	Row.PosX = Loc.X;
	Row.PosY = Loc.Y;
	Row.NodeType = NodeType;
	Row.BattleMapName = BattleMapName;
	Row.StarRating = StarRating;
	Row.bCanEscape = bCanEscape;
	Row.BaseGoldReward = BaseGoldReward;
	Row.BaseFoodReward = BaseFoodReward;
	Row.EventID = EventID;
	Row.PrerequisiteNodeIDs = PrerequisiteNodeIDs;
	return Row;
}

FVector2D ALFPWorldMapNode::GetPosition2D() const
{
	FVector Loc = GetActorLocation();
	return FVector2D(Loc.X, Loc.Y);
}

void ALFPWorldMapNode::SetPosition2D(FVector2D NewPos)
{
	SetActorLocation(FVector(NewPos.X, NewPos.Y, 0.f));
}

void ALFPWorldMapNode::SetNodeVisualData(ULFPWorldNodeDataAsset* InData)
{
	NodeVisualData = InData;
	UpdateVisualState();
}

void ALFPWorldMapNode::UpdateVisualState()
{
	if (!NodeVisualData || !NodeSpriteComponent) return;

	// 未揭开 → 显示迷雾精灵（如果有）
	if (!bIsRevealed && NodeVisualData->FogSprite)
	{
		NodeSpriteComponent->SetSprite(NodeVisualData->FogSprite);
		return;
	}

	// 已触发 → 显示已触发精灵（如果有）
	if (bHasBeenTriggered && NodeVisualData->TriggeredSprite)
	{
		NodeSpriteComponent->SetSprite(NodeVisualData->TriggeredSprite);
		return;
	}

	// 默认精灵
	NodeSpriteComponent->SetSprite(NodeVisualData->DefaultSprite);
}

void ALFPWorldMapNode::SetHighlighted(bool bHighlight)
{
	if (HighlightSpriteComponent)
	{
		HighlightSpriteComponent->SetVisibility(bHighlight);
	}
}
