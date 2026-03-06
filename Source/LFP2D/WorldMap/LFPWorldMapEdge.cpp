#include "LFP2D/WorldMap/LFPWorldMapEdge.h"
#include "PaperSpriteComponent.h"

ALFPWorldMapEdge::ALFPWorldMapEdge()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootSceneComponent);

	EdgeSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EdgeSprite"));
	EdgeSpriteComponent->SetupAttachment(RootSceneComponent);
	// 边在节点下方渲染（Z 略低），旋转使其平躺在 XY 平面
	EdgeSpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, -2.f));
	EdgeSpriteComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

void ALFPWorldMapEdge::InitFromRowData(const FLFPWorldEdgeRow& Row)
{
	FromNodeID = Row.FromNodeID;
	ToNodeID = Row.ToNodeID;
	TravelTurnCost = Row.TravelTurnCost;
}

FLFPWorldEdgeRow ALFPWorldMapEdge::ExportToRowData() const
{
	FLFPWorldEdgeRow Row;
	Row.FromNodeID = FromNodeID;
	Row.ToNodeID = ToNodeID;
	Row.TravelTurnCost = TravelTurnCost;
	return Row;
}

void ALFPWorldMapEdge::UpdateVisualPosition(FVector StartPos, FVector EndPos)
{
	CachedStartPos = StartPos;
	CachedEndPos = EndPos;

	// 计算中点作为 Actor 位置
	FVector Midpoint = (StartPos + EndPos) * 0.5f;
	Midpoint.Z = -2.f; // 边在节点下方
	SetActorLocation(Midpoint);

	// 计算方向和距离
	FVector Direction = EndPos - StartPos;
	float Distance = Direction.Size2D();

	// 计算旋转角度（XY 平面内）
	float AngleRad = FMath::Atan2(Direction.Y, Direction.X);
	SetActorRotation(FRotator(0.f, FMath::RadiansToDegrees(AngleRad), 0.f));

	// 拉伸精灵以匹配距离（假设精灵原始宽度为 1 单位）
	if (EdgeSpriteComponent)
	{
		// X 缩放 = 距离，Y 缩放保持线条宽度
		EdgeSpriteComponent->SetRelativeScale3D(FVector(Distance / 100.f, 1.f, 1.f));
	}
}

void ALFPWorldMapEdge::SetEdgeSprite(UPaperSprite* InSprite)
{
	if (EdgeSpriteComponent)
	{
		EdgeSpriteComponent->SetSprite(InSprite);
	}
}

void ALFPWorldMapEdge::SetHighlighted(bool bHighlight)
{
	if (EdgeSpriteComponent)
	{
		// 高亮时改变颜色
		EdgeSpriteComponent->SetSpriteColor(bHighlight ? FLinearColor::Yellow : FLinearColor::White);
	}
}
