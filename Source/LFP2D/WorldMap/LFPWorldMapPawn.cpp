#include "LFP2D/WorldMap/LFPWorldMapPawn.h"
#include "PaperSpriteComponent.h"
#include "Curves/CurveFloat.h"

ALFPWorldMapPawn::ALFPWorldMapPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(Root);

	PawnSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PawnSprite"));
	PawnSpriteComponent->SetupAttachment(Root);
	// 与节点精灵相同的朝向：平躺在 XY 平面
	PawnSpriteComponent->SetRelativeRotation(FRotator(0.f, 90.f, -90.f));
	// 棋子渲染在节点上方
	PawnSpriteComponent->TranslucencySortPriority = 10;
}

void ALFPWorldMapPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MoveTimeline.IsPlaying())
	{
		MoveTimeline.TickTimeline(DeltaTime);
	}
}

void ALFPWorldMapPawn::MoveToLocation(FVector TargetLocation)
{
	if (bIsMoving) return;
	if (!MoveCurve)
	{
		// 没有曲线，直接瞬移
		SetActorLocation(TargetLocation);
		OnMoveComplete.Broadcast();
		return;
	}

	bIsMoving = true;
	MoveStartLocation = GetActorLocation();
	MoveTargetLocation = TargetLocation;

	// 设置 Timeline
	FOnTimelineFloat UpdateCallback;
	UpdateCallback.BindUFunction(this, FName("UpdateMoveAnimation"));

	FOnTimelineEvent FinishedCallback;
	FinishedCallback.BindUFunction(this, FName("OnMoveFinished"));

	MoveTimeline.AddInterpFloat(MoveCurve, UpdateCallback);
	MoveTimeline.SetTimelineFinishedFunc(FinishedCallback);
	MoveTimeline.SetPlayRate(1.0f / MoveDuration);
	MoveTimeline.PlayFromStart();
}

void ALFPWorldMapPawn::SetLocationImmediate(FVector Location)
{
	SetActorLocation(Location);
}

void ALFPWorldMapPawn::UpdateMoveAnimation(float Alpha)
{
	FVector NewLocation = FMath::Lerp(MoveStartLocation, MoveTargetLocation, Alpha);
	SetActorLocation(NewLocation);
}

void ALFPWorldMapPawn::OnMoveFinished()
{
	// 确保精确到达目标位置
	SetActorLocation(MoveTargetLocation);
	bIsMoving = false;

	// 清理 Timeline（防止下次移动时叠加回调）
	MoveTimeline.SetTimelineFinishedFunc(FOnTimelineEvent());
	// 清理所有 InterpFloat
	MoveTimeline = FTimeline();

	OnMoveComplete.Broadcast();
}
