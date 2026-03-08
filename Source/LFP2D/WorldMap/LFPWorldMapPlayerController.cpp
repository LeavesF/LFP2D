#include "LFP2D/WorldMap/LFPWorldMapPlayerController.h"
#include "LFP2D/WorldMap/LFPWorldMapManager.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "LFP2D/WorldMap/LFPWorldMapPlayerState.h"
#include "LFP2D/WorldMap/LFPWorldMapEditorComponent.h"
#include "LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ALFPWorldMapPlayerController::ALFPWorldMapPlayerController()
{
	CameraOffset = FVector::ZeroVector;

	// 创建世界地图编辑器组件
	WorldMapEditorComponent = CreateDefaultSubobject<ULFPWorldMapEditorComponent>(TEXT("WorldMapEditorComponent"));
}

void ALFPWorldMapPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 设置输入模式
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	// 设置 Enhanced Input
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultInputMapping)
		{
			Subsystem->AddMappingContext(DefaultInputMapping, 0);
		}
	}
}

void ALFPWorldMapPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDragging)
	{
		DragTime += DeltaTime;
	}

	// 更新相机位置和旋转
	// 相机位置：CameraOffset（XY 平移）+ CurrentZoom（高度）
	FVector TargetLocation = CameraOffset + FVector(0.f, 0.f, CurrentZoom);

	// 相机旋转：俯视角
	FRotator TargetRotation(CameraPitchAngle, 0.f, 0.f);

	// 平滑插值并设置
	FRotator CurrentRotation = GetControlRotation();
	SetControlRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f));

	// 通过 Pawn 控制相机位置（如果有 Pawn）
	if (APawn* ControlledPawn = GetPawn())
	{
		FVector CurrentLocation = ControlledPawn->GetActorLocation();
		ControlledPawn->SetActorLocation(
			FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 5.0f)
		);
	}
}

void ALFPWorldMapPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 基础操作
		EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Completed, this, &ALFPWorldMapPlayerController::OnConfirmAction);
		EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Completed, this, &ALFPWorldMapPlayerController::OnCancelAction);

		// 相机操作
		EnhancedInputComponent->BindAction(CameraPanAction, ETriggerEvent::Triggered, this, &ALFPWorldMapPlayerController::OnCameraPan);
		EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Started, this, &ALFPWorldMapPlayerController::OnCameraDragStarted);
		EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Triggered, this, &ALFPWorldMapPlayerController::OnCameraDragTriggered);
		EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Completed, this, &ALFPWorldMapPlayerController::OnCameraDragCompleted);
		EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &ALFPWorldMapPlayerController::OnCameraZoom);

		// 编辑器切换
		EnhancedInputComponent->BindAction(ToggleEditorAction, ETriggerEvent::Started, this, &ALFPWorldMapPlayerController::OnToggleEditorAction);
	}
}

// ============== 输入处理 ==============

void ALFPWorldMapPlayerController::OnConfirmAction(const FInputActionValue& Value)
{
	bIsDragging = false;
	if (DragTime > DragThresholdTime)
	{
		DragTime = 0.f;
		return;
	}
	DragTime = 0.f;

	// 编辑器模式优先处理
	if (WorldMapEditorComponent && WorldMapEditorComponent->IsEditorActive())
	{
		// 射线检测点击的节点
		FHitResult HitResult;
		GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
		ALFPWorldMapNode* ClickedNode = Cast<ALFPWorldMapNode>(HitResult.GetActor());

		ELFPWorldMapEditorTool CurrentTool = WorldMapEditorComponent->GetCurrentTool();

		switch (CurrentTool)
		{
		case ELFPWorldMapEditorTool::WMET_PlaceNode:
			{
				// 射线投射到 Z=0 平面获取世界坐标
				FVector WorldOrigin, WorldDirection;
				DeprojectMousePositionToWorld(WorldOrigin, WorldDirection);
				if (FMath::Abs(WorldDirection.Z) > KINDA_SMALL_NUMBER)
				{
					float T = -WorldOrigin.Z / WorldDirection.Z;
					FVector HitPoint = WorldOrigin + WorldDirection * T;
					WorldMapEditorComponent->PlaceNodeAt(FVector2D(HitPoint.X, HitPoint.Y));
				}
			}
			break;

		case ELFPWorldMapEditorTool::WMET_RemoveNode:
			if (ClickedNode)
			{
				WorldMapEditorComponent->SelectNode(ClickedNode);
				WorldMapEditorComponent->RemoveSelectedNode();
			}
			break;

		case ELFPWorldMapEditorTool::WMET_MoveNode:
			if (ClickedNode)
			{
				WorldMapEditorComponent->SelectNode(ClickedNode);
			}
			else if (WorldMapEditorComponent->GetSelectedNode())
			{
				// 移动到点击位置
				FVector WorldOrigin, WorldDirection;
				DeprojectMousePositionToWorld(WorldOrigin, WorldDirection);
				if (FMath::Abs(WorldDirection.Z) > KINDA_SMALL_NUMBER)
				{
					float T = -WorldOrigin.Z / WorldDirection.Z;
					FVector HitPoint = WorldOrigin + WorldDirection * T;
					WorldMapEditorComponent->MoveSelectedNodeTo(FVector2D(HitPoint.X, HitPoint.Y));
				}
			}
			break;

		case ELFPWorldMapEditorTool::WMET_ConnectEdge:
			if (ClickedNode)
			{
				WorldMapEditorComponent->HandleEdgeConnection(ClickedNode);
			}
			break;

		case ELFPWorldMapEditorTool::WMET_RemoveEdge:
			if (ClickedNode)
			{
				WorldMapEditorComponent->HandleEdgeRemoval(ClickedNode);
			}
			break;

		case ELFPWorldMapEditorTool::WMET_SetNodeParams:
			if (ClickedNode)
			{
				WorldMapEditorComponent->SelectNode(ClickedNode);
				WorldMapEditorComponent->ApplyParamsToSelectedNode();
			}
			break;

		default:
			break;
		}
		return;
	}

	// 游戏模式：点击节点移动或进入
	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	ALFPWorldMapNode* ClickedNode = Cast<ALFPWorldMapNode>(HitResult.GetActor());

	if (ClickedNode)
	{
		ALFPWorldMapManager* Manager = GetWorldMapManager();
		if (!Manager || !Manager->GetPlayerState()) return;

		int32 CurrentNodeID = Manager->GetPlayerState()->CurrentNodeID;

		// 点击当前节点 → 进入节点
		if (ClickedNode->NodeID == CurrentNodeID)
		{
			EnterNode(ClickedNode);
			return;
		}

		// 点击相邻可达节点 → 移动
		TArray<int32> ReachableIDs = Manager->GetPlayerState()->GetReachableNodeIDs(Manager);
		if (ReachableIDs.Contains(ClickedNode->NodeID))
		{
			MoveToNode(ClickedNode);
		}
		else
		{
			// 不可达，只选中
			SelectNode(ClickedNode);
		}
	}
}

void ALFPWorldMapPlayerController::OnCancelAction(const FInputActionValue& Value)
{
	// 取消选中和可达高亮
	ClearReachableHighlight();
	if (SelectedNode)
	{
		SelectedNode->SetHighlighted(false);
		SelectedNode = nullptr;
	}
}

void ALFPWorldMapPlayerController::OnToggleEditorAction(const FInputActionValue& Value)
{
	if (!WorldMapEditorComponent) return;

	WorldMapEditorComponent->ToggleEditorMode();

	if (WorldMapEditorComponent->IsEditorActive())
	{
		// 创建或显示编辑器 Widget
		if (!WorldMapEditorWidget && WorldMapEditorWidgetClass)
		{
			WorldMapEditorWidget = CreateWidget<ULFPWorldMapEditorWidget>(this, WorldMapEditorWidgetClass);
			if (WorldMapEditorWidget)
			{
				WorldMapEditorWidget->AddToViewport();
				WorldMapEditorWidget->InitializeEditor(WorldMapEditorComponent);
			}
		}
		else if (WorldMapEditorWidget)
		{
			WorldMapEditorWidget->SetVisibility(ESlateVisibility::Visible);
		}

		bShowMouseCursor = true;
	}
	else
	{
		// 隐藏编辑器 Widget
		if (WorldMapEditorWidget)
		{
			WorldMapEditorWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// ============== 相机操作 ==============

void ALFPWorldMapPlayerController::OnCameraPan(const FInputActionValue& Value)
{
	FVector2D PanInput = Value.Get<FVector2D>();
	CameraOffset += FVector(PanInput.X, PanInput.Y, 0) * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
}

void ALFPWorldMapPlayerController::OnCameraDragStarted(const FInputActionValue& Value)
{
	bIsDragging = true;
	DragTime = 0.f;
	GetMousePosition(DragStartPosition.X, DragStartPosition.Y);
}

void ALFPWorldMapPlayerController::OnCameraDragTriggered(const FInputActionValue& Value)
{
	if (!bIsDragging) return;

	FVector2D CurrentMousePos;
	GetMousePosition(CurrentMousePos.X, CurrentMousePos.Y);
	FVector2D Delta = CurrentMousePos - DragStartPosition;

	CameraOffset -= FVector(-Delta.Y, Delta.X, 0) * CameraDragSpeed;
	DragStartPosition = CurrentMousePos;
}

void ALFPWorldMapPlayerController::OnCameraDragCompleted(const FInputActionValue& Value)
{
	bIsDragging = false;
}

void ALFPWorldMapPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	float ZoomDelta = Value.Get<float>();
	CurrentZoom = FMath::Clamp(CurrentZoom - ZoomDelta * CameraZoomSpeed, MinZoomDistance, MaxZoomDistance);

	// 应用缩放到相机
	if (APlayerCameraManager* CameraManager = PlayerCameraManager)
	{
		FVector CameraLocation = CameraManager->GetCameraLocation();
		CameraLocation.Z = CurrentZoom;
		CameraManager->SetManualCameraFade(0.f, FLinearColor::Black, false);
	}
}

// ============== 节点交互 ==============

void ALFPWorldMapPlayerController::SelectNode(ALFPWorldMapNode* Node)
{
	// 取消之前的选中
	if (SelectedNode)
	{
		SelectedNode->SetHighlighted(false);
	}

	SelectedNode = Node;

	if (SelectedNode)
	{
		SelectedNode->SetHighlighted(true);
	}
}

void ALFPWorldMapPlayerController::MoveToNode(ALFPWorldMapNode* Node)
{
	if (!Node) return;

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	// 棋子移动中不允许操作
	if (Manager->IsPawnMoving()) return;

	ClearReachableHighlight();

	bool bMoved = Manager->MovePlayer(Node->NodeID);
	if (bMoved)
	{
		// 移动后显示新的可达节点
		ShowReachableNodes();
	}
}

void ALFPWorldMapPlayerController::ShowReachableNodes()
{
	ClearReachableHighlight();

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager || !Manager->GetPlayerState()) return;

	TArray<int32> ReachableIDs = Manager->GetPlayerState()->GetReachableNodeIDs(Manager);
	for (int32 NodeID : ReachableIDs)
	{
		if (ALFPWorldMapNode* Node = Manager->GetNode(NodeID))
		{
			Node->SetHighlighted(true);
			ReachableNodes.Add(Node);
		}
	}
}

void ALFPWorldMapPlayerController::ClearReachableHighlight()
{
	for (ALFPWorldMapNode* Node : ReachableNodes)
	{
		if (Node)
		{
			Node->SetHighlighted(false);
		}
	}
	ReachableNodes.Empty();
}

void ALFPWorldMapPlayerController::EnterNode(ALFPWorldMapNode* Node)
{
	if (!Node) return;

	UE_LOG(LogTemp, Log, TEXT("进入节点 %d，类型: %d"), Node->NodeID, (int32)Node->NodeType);

	switch (Node->NodeType)
	{
	case ELFPWorldNodeType::WNT_Battle:
	case ELFPWorldNodeType::WNT_Boss:
		EnterBattle(Node);
		break;

	case ELFPWorldNodeType::WNT_Event:
		// TODO: 触发事件系统
		UE_LOG(LogTemp, Log, TEXT("事件节点 %d: EventID=%s"), Node->NodeID, *Node->EventID);
		break;

	case ELFPWorldNodeType::WNT_Shop:
		// TODO: 打开商店 UI
		UE_LOG(LogTemp, Log, TEXT("商店节点 %d"), Node->NodeID);
		break;

	case ELFPWorldNodeType::WNT_Town:
		// TODO: 进入城镇
		UE_LOG(LogTemp, Log, TEXT("城镇节点 %d"), Node->NodeID);
		break;

	case ELFPWorldNodeType::WNT_QuestNPC:
		// TODO: 触发任务对话
		UE_LOG(LogTemp, Log, TEXT("任务NPC节点 %d"), Node->NodeID);
		break;

	case ELFPWorldNodeType::WNT_SkillNode:
		// TODO: 打开技能树 UI
		UE_LOG(LogTemp, Log, TEXT("技能节点 %d"), Node->NodeID);
		break;

	default:
		break;
	}
}

void ALFPWorldMapPlayerController::EnterBattle(ALFPWorldMapNode* BattleNode)
{
	if (!BattleNode) return;

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	// 保存世界地图快照
	FLFPWorldMapSnapshot Snapshot;
	Snapshot.WorldMapName = Manager->GetCurrentWorldMapName();
	if (ULFPWorldMapPlayerState* PS = Manager->GetPlayerState())
	{
		Snapshot.CurrentNodeID = PS->CurrentNodeID;
		Snapshot.CurrentTurn = PS->CurrentTurn;
		Snapshot.VisitedNodeIDs = PS->VisitedNodeIDs;
		Snapshot.RevealedNodeIDs = PS->RevealedNodeIDs;
	}

	// 收集已触发节点
	for (const auto& Pair : Manager->GetNodeMap())
	{
		if (Pair.Value && Pair.Value->bHasBeenTriggered)
		{
			Snapshot.TriggeredNodeIDs.Add(Pair.Key);
		}
	}

	GI->SaveWorldMapSnapshot(Snapshot);

	// 设置战斗请求
	FLFPBattleRequest Request;
	Request.SourceNodeID = BattleNode->NodeID;
	Request.BattleMapName = BattleNode->BattleMapName;
	Request.StarRating = BattleNode->StarRating;
	Request.bCanEscape = BattleNode->bCanEscape;
	GI->SetBattleRequest(Request);

	// 切换到战斗关卡
	GI->TransitionToBattle(TEXT(""));
}

ALFPWorldMapManager* ALFPWorldMapPlayerController::GetWorldMapManager() const
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
