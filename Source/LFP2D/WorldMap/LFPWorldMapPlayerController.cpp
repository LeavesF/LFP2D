#include "LFP2D/WorldMap/LFPWorldMapPlayerController.h"
#include "LFP2D/WorldMap/LFPWorldMapManager.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "LFP2D/WorldMap/LFPWorldMapPawn.h"
#include "LFP2D/WorldMap/LFPWorldMapPlayerState.h"
#include "LFP2D/WorldMap/LFPWorldMapEditorComponent.h"
#include "LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h"
#include "LFP2D/UI/WorldMap/LFPUnitMergeWidget.h"
#include "LFP2D/UI/WorldMap/LFPShopWidget.h"
#include "LFP2D/UI/WorldMap/LFPHireMarketWidget.h"
#include "LFP2D/UI/WorldMap/LFPTeleportWidget.h"
#include "LFP2D/UI/WorldMap/LFPWorldMapHUDWidget.h"
#include "LFP2D/UI/Town/LFPTownWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
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

	// 场景切换淡入效果
	if (PlayerCameraManager)
	{
		ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
		float FadeDuration = GI ? GI->TransitionFadeDuration : 0.5f;
		PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeDuration, FLinearColor::Black, false, false);
	}

	// 首帧跳过相机平滑插值
	bSnapCameraNextFrame = true;

	// 尝试初始化 HUD（PlayerState 可能稍后才就绪）
	TryInitializeHUD();

	// 初始化当前所在节点
	if (ALFPWorldMapManager* Manager = GetWorldMapManager())
	{
		if (ULFPWorldMapPlayerState* PS = Manager->GetPlayerState())
		{
			CurrentNode = Manager->GetNode(PS->CurrentNodeID);
		}
	}
}

void ALFPWorldMapPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 延迟初始化 HUD（等待 PlayerState 就绪）
	if (!bHUDInitialized)
	{
		TryInitializeHUD();
	}

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
	if (bSnapCameraNextFrame)
	{
		SetControlRotation(TargetRotation);
	}
	else
	{
		SetControlRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f));
	}

	// 通过 Pawn 控制相机位置（如果有 Pawn）
	if (APawn* ControlledPawn = GetPawn())
	{
		if (bSnapCameraNextFrame)
		{
			// 首帧直接跳到目标位置，避免切场景后相机大幅滑动
			ControlledPawn->SetActorLocation(TargetLocation);
			bSnapCameraNextFrame = false;
		}
		else
		{
			FVector CurrentLocation = ControlledPawn->GetActorLocation();
			ControlledPawn->SetActorLocation(
				FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 5.0f)
			);
		}
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

		// HUD 切换
		EnhancedInputComponent->BindAction(ToggleHUDAction, ETriggerEvent::Started, this, &ALFPWorldMapPlayerController::OnToggleHUDAction);
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

	// 游戏模式：棋子移动中屏蔽点击
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (Manager && Manager->IsPawnMoving()) return;

	// 游戏模式：点击节点移动或进入
	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	ALFPWorldMapNode* ClickedNode = Cast<ALFPWorldMapNode>(HitResult.GetActor());

	if (ClickedNode)
	{
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

void ALFPWorldMapPlayerController::OnToggleHUDAction(const FInputActionValue& Value)
{
	if (!WorldMapHUDWidget) return;

	if (WorldMapHUDWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		WorldMapHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		WorldMapHUDWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ALFPWorldMapPlayerController::TryInitializeHUD()
{
	if (bHUDInitialized) return;
	if (!WorldMapHUDWidgetClass) return;

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	ULFPWorldMapPlayerState* PS = Manager->GetPlayerState();
	if (!PS) return;

	// 创建 HUD Widget
	WorldMapHUDWidget = CreateWidget<ULFPWorldMapHUDWidget>(this, WorldMapHUDWidgetClass);
	if (WorldMapHUDWidget)
	{
		WorldMapHUDWidget->AddToViewport(0);
		WorldMapHUDWidget->Setup(GI, PS);
	}

	bHUDInitialized = true;
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

	// 记录目标节点，移动完成后自动进入
	PendingEnterNode = Node;

	// 绑定棋子移动完成回调
	if (ALFPWorldMapPawn* MapPawn = Manager->GetPlayerPawn())
	{
		MapPawn->OnMoveComplete.AddUniqueDynamic(this, &ALFPWorldMapPlayerController::OnPawnMoveCompleted);
	}

	bool bMoved = Manager->MovePlayer(Node->NodeID);
	if (!bMoved)
	{
		PendingEnterNode = nullptr;
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
		OpenShop(Node, false);
		break;

	case ELFPWorldNodeType::WNT_HireMarket:
		OpenHireMarket(Node, false);
		break;

	case ELFPWorldNodeType::WNT_Town:
		OpenTown(Node);
		break;

	case ELFPWorldNodeType::WNT_QuestNPC:
		// TODO: 触发任务对话
		UE_LOG(LogTemp, Log, TEXT("任务NPC节点 %d"), Node->NodeID);
		break;

	case ELFPWorldNodeType::WNT_SkillNode:
		// TODO: 打开技能树 UI
		UE_LOG(LogTemp, Log, TEXT("技能节点 %d"), Node->NodeID);
		break;

	case ELFPWorldNodeType::WNT_EvolutionTower:
		OpenEvolutionTower();
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
	Request.BaseGoldReward = BattleNode->BaseGoldReward;
	Request.BaseFoodReward = BattleNode->BaseFoodReward;
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

void ALFPWorldMapPlayerController::OnPawnMoveCompleted()
{
	// 解绑回调
	if (ALFPWorldMapManager* Manager = GetWorldMapManager())
	{
		if (ALFPWorldMapPawn* MapPawn = Manager->GetPlayerPawn())
		{
			MapPawn->OnMoveComplete.RemoveDynamic(this, &ALFPWorldMapPlayerController::OnPawnMoveCompleted);
		}
	}

	// 显示新的可达节点
	ShowReachableNodes();

	// 更新当前所在节点
	if (ALFPWorldMapManager* Manager = GetWorldMapManager())
	{
		if (ULFPWorldMapPlayerState* PS = Manager->GetPlayerState())
		{
			CurrentNode = Manager->GetNode(PS->CurrentNodeID);
		}
	}

	// 自动进入目标节点
	if (PendingEnterNode)
	{
		ALFPWorldMapNode* NodeToEnter = PendingEnterNode;
		PendingEnterNode = nullptr;
		EnterNode(NodeToEnter);
	}
}

void ALFPWorldMapPlayerController::OpenEvolutionTower()
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !UnitMergeWidgetClass) return;

	if (!UnitMergeWidget)
	{
		UnitMergeWidget = CreateWidget<ULFPUnitMergeWidget>(this, UnitMergeWidgetClass);
		if (UnitMergeWidget)
		{
			UnitMergeWidget->OnClosed.AddDynamic(this, &ALFPWorldMapPlayerController::OnUnitMergeWidgetClosed);
			UnitMergeWidget->AddToViewport();
		}
	}
	else
	{
		UnitMergeWidget->SetVisibility(ESlateVisibility::Visible);
		UnitMergeWidget->AddToViewport();
	}

	if (UnitMergeWidget)
	{
		UnitMergeWidget->Setup(GI, GI->UnitRegistry);
	}
}

void ALFPWorldMapPlayerController::OnUnitMergeWidgetClosed()
{
	UE_LOG(LogTemp, Log, TEXT("升华塔: 升阶面板已关闭"));

	// 如果是从城镇里打开的升华塔，关闭后重新显示城镇面板
	if (TownWidget && TownWidget->IsInViewport())
	{
		TownWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ALFPWorldMapPlayerController::OpenShop(ALFPWorldMapNode* ShopNode, bool bReturnToTownOnClose)
{
	if (!ShopNode || !ShopWidgetClass)
	{
		return;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || ShopNode->ShopID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("商店节点 %d: ShopID 无效"), ShopNode->NodeID);
		if (bReturnToTownOnClose && TownWidget && TownWidget->IsInViewport())
		{
			TownWidget->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	FLFPShopDefinition ShopDefinition;
	if (!GI->FindShopDefinition(ShopNode->ShopID, ShopDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("商店配置不存在: %s"), *ShopNode->ShopID.ToString());
		if (bReturnToTownOnClose && TownWidget && TownWidget->IsInViewport())
		{
			TownWidget->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	bReturnToTownAfterShopClose = bReturnToTownOnClose;

	if (!ShopWidget)
	{
		ShopWidget = CreateWidget<ULFPShopWidget>(this, ShopWidgetClass);
		if (ShopWidget)
		{
			ShopWidget->OnClosed.AddDynamic(this, &ALFPWorldMapPlayerController::OnShopWidgetClosed);
			ShopWidget->AddToViewport();
		}
	}
	else
	{
		ShopWidget->SetVisibility(ESlateVisibility::Visible);
		ShopWidget->AddToViewport();
	}

	if (ShopWidget)
	{
		ShopWidget->Setup(GI, ShopNode->ShopID, ShopDefinition);
	}
}

void ALFPWorldMapPlayerController::OnShopWidgetClosed()
{
	UE_LOG(LogTemp, Log, TEXT("商店: 商店面板已关闭"));

	if (bReturnToTownAfterShopClose && TownWidget && TownWidget->IsInViewport())
	{
		TownWidget->SetVisibility(ESlateVisibility::Visible);
	}

	bReturnToTownAfterShopClose = false;
}

void ALFPWorldMapPlayerController::OpenHireMarket(ALFPWorldMapNode* HireMarketNode, bool bReturnToTownOnClose)
{
	if (!HireMarketNode || !HireMarketWidgetClass)
	{
		return;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || HireMarketNode->HireMarketID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("雇佣市场节点 %d: HireMarketID 无效"), HireMarketNode->NodeID);
		if (bReturnToTownOnClose && TownWidget && TownWidget->IsInViewport())
		{
			TownWidget->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	FLFPHireMarketDefinition HireMarketDefinition;
	if (!GI->FindHireMarketDefinition(HireMarketNode->HireMarketID, HireMarketDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("雇佣市场配置不存在: %s"), *HireMarketNode->HireMarketID.ToString());
		if (bReturnToTownOnClose && TownWidget && TownWidget->IsInViewport())
		{
			TownWidget->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	bReturnToTownAfterHireMarketClose = bReturnToTownOnClose;

	if (!HireMarketWidget)
	{
		HireMarketWidget = CreateWidget<ULFPHireMarketWidget>(this, HireMarketWidgetClass);
		if (HireMarketWidget)
		{
			HireMarketWidget->OnClosed.AddDynamic(this, &ALFPWorldMapPlayerController::OnHireMarketWidgetClosed);
			HireMarketWidget->AddToViewport();
		}
	}
	else
	{
		HireMarketWidget->SetVisibility(ESlateVisibility::Visible);
		HireMarketWidget->AddToViewport();
	}

	if (HireMarketWidget)
	{
		HireMarketWidget->Setup(GI, HireMarketNode->HireMarketID, HireMarketDefinition);
	}
}

void ALFPWorldMapPlayerController::OnHireMarketWidgetClosed()
{
	UE_LOG(LogTemp, Log, TEXT("雇佣市场: 面板已关闭"));

	if (bReturnToTownAfterHireMarketClose && TownWidget && TownWidget->IsInViewport())
	{
		TownWidget->SetVisibility(ESlateVisibility::Visible);
	}

	bReturnToTownAfterHireMarketClose = false;
}

void ALFPWorldMapPlayerController::OpenTown(ALFPWorldMapNode* TownNode)
{
	if (!TownNode || !TownWidgetClass) return;

	// 解析建筑列表
	TArray<ELFPTownBuildingType> BuildingTypes = LFPTownUtils::ParseBuildingList(TownNode->TownBuildingList);

	if (BuildingTypes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("城镇节点 %d: 建筑列表为空"), TownNode->NodeID);
		return;
	}

	// 创建或显示 TownWidget
	if (!TownWidget)
	{
		TownWidget = CreateWidget<ULFPTownWidget>(this, TownWidgetClass);
		if (TownWidget)
		{
			TownWidget->OnClosed.AddDynamic(this, &ALFPWorldMapPlayerController::OnTownWidgetClosed);
			TownWidget->OnBuildingRequested.AddDynamic(this, &ALFPWorldMapPlayerController::OnTownBuildingRequested);
			TownWidget->AddToViewport();
		}
	}
	else
	{
		TownWidget->SetVisibility(ESlateVisibility::Visible);
		TownWidget->AddToViewport();
	}

	if (TownWidget)
	{
		TownWidget->Setup(BuildingTypes);
	}

	UE_LOG(LogTemp, Log, TEXT("城镇节点 %d: 打开城镇面板，建筑数量 %d"), TownNode->NodeID, BuildingTypes.Num());
}

void ALFPWorldMapPlayerController::OnTownWidgetClosed()
{
	UE_LOG(LogTemp, Log, TEXT("城镇: 城镇面板已关闭"));
}

void ALFPWorldMapPlayerController::OnTownBuildingRequested(ELFPTownBuildingType BuildingType)
{
	UE_LOG(LogTemp, Log, TEXT("城镇: 请求打开建筑 %s"), *LFPTownUtils::BuildingTypeToString(BuildingType));

	switch (BuildingType)
	{
	case ELFPTownBuildingType::TBT_EvolutionTower:
		// 隐藏城镇面板，打开升华塔
		if (TownWidget)
		{
			TownWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		OpenEvolutionTower();
		break;

	case ELFPTownBuildingType::TBT_Shop:
		if (TownWidget)
		{
			TownWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		if (CurrentNode)
		{
			OpenShop(CurrentNode, true);
		}
		break;

	case ELFPTownBuildingType::TBT_HireMarket:
		if (TownWidget)
		{
			TownWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		if (CurrentNode)
		{
			OpenHireMarket(CurrentNode, true);
		}
		break;

	case ELFPTownBuildingType::TBT_Teleport:
		if (TownWidget)
		{
			TownWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		OpenTeleport();
		break;

	case ELFPTownBuildingType::TBT_QuestNPC:
		// TODO: 打开任务对话
		UE_LOG(LogTemp, Log, TEXT("城镇: 任务NPC功能尚未实现"));
		break;

	case ELFPTownBuildingType::TBT_SkillNode:
		// TODO: 打开技能树 UI
		UE_LOG(LogTemp, Log, TEXT("城镇: 技能节点功能尚未实现"));
		break;

	default:
		break;
	}
}

// ============== 传送阵 ==============

void ALFPWorldMapPlayerController::OpenTeleport()
{
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager || !TeleportWidgetClass) return;

	ULFPWorldMapPlayerState* PS = Manager->GetPlayerState();
	if (!PS) return;

	// 获取可传送目的地
	TArray<ALFPWorldMapNode*> Destinations = Manager->GetTeleportDestinations(PS->CurrentNodeID);

	// 创建或显示 TeleportWidget
	if (!TeleportWidget)
	{
		TeleportWidget = CreateWidget<ULFPTeleportWidget>(this, TeleportWidgetClass);
		if (TeleportWidget)
		{
			TeleportWidget->OnTeleportTargetSelected.AddDynamic(this, &ALFPWorldMapPlayerController::OnTeleportTargetSelected);
			TeleportWidget->OnClosed.AddDynamic(this, &ALFPWorldMapPlayerController::OnTeleportWidgetClosed);
			TeleportWidget->AddToViewport();
		}
	}
	else
	{
		TeleportWidget->SetVisibility(ESlateVisibility::Visible);
		TeleportWidget->AddToViewport();
	}

	if (TeleportWidget)
	{
		TeleportWidget->Setup(Destinations);
	}

	UE_LOG(LogTemp, Log, TEXT("传送阵: 打开面板，可选目的地 %d 个"), Destinations.Num());
}

void ALFPWorldMapPlayerController::OnTeleportTargetSelected(int32 TargetNodeID)
{
	ALFPWorldMapManager* Manager = GetWorldMapManager();
	if (!Manager) return;

	// 关闭传送 UI
	if (TeleportWidget)
	{
		TeleportWidget->SetVisibility(ESlateVisibility::Collapsed);
		TeleportWidget->RemoveFromParent();
	}

	// 关闭城镇面板
	if (TownWidget)
	{
		TownWidget->SetVisibility(ESlateVisibility::Collapsed);
		TownWidget->RemoveFromParent();
	}

	// 执行传送
	bool bSuccess = Manager->TeleportPlayerToNode(TargetNodeID);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("传送阵: 传送到节点 %d 失败"), TargetNodeID);
		return;
	}

	// 传送后自动进入目标城镇
	ALFPWorldMapNode* TargetNode = Manager->GetNode(TargetNodeID);
	if (TargetNode)
	{
		CurrentNode = TargetNode;
		SelectedNode = TargetNode;
		OpenTown(TargetNode);
	}

	UE_LOG(LogTemp, Log, TEXT("传送阵: 已传送到节点 %d 并进入城镇"), TargetNodeID);
}

void ALFPWorldMapPlayerController::OnTeleportWidgetClosed()
{
	UE_LOG(LogTemp, Log, TEXT("传送阵: 面板已关闭"));

	// 返回城镇面板
	if (TownWidget && TownWidget->IsInViewport())
	{
		TownWidget->SetVisibility(ESlateVisibility::Visible);
	}
}
