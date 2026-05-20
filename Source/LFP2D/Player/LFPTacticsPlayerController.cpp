// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Card/LFPBattleCardComponent.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Unit/Betrayal/LFPBetrayalWorldSubsystem.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/UI/Fighting/LFPBattleHUDWidget.h"
#include "LFP2D/UI/Fighting/LFPCardItemWidget.h"
#include "LFP2D/UI/Fighting/LFPCurrentUnitInfoWidget.h"
#include "LFP2D/UI/Fighting/LFPCardHandWidget.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "LFP2D/UI/Fighting/LFPSkillSelectionWidget.h"
#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/UI/Fighting/LFPBattleResultWidget.h"
#include "LFP2D/HexGrid/LFPMapEditorComponent.h"
#include "LFP2D/UI/MapEditor/LFPMapEditorWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
//#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Widget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 MovementActionPointCost = 1;

	bool IsVisibleAndHovered(const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}

		const ESlateVisibility Visibility = Widget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed &&
			Visibility != ESlateVisibility::Hidden &&
			Widget->IsHovered();
	}

	bool CanControlPlayerUnit(const ALFPTacticsUnit* Unit, const ALFPTurnManager* TurnManager)
	{
		return Unit &&
			TurnManager &&
			TurnManager->GetCurrentPhase() == EBattlePhase::BP_PlayerActionPhase &&
			Unit->IsAlive() &&
			Unit->GetAffiliation() == EUnitAffiliation::UA_Player &&
			!Unit->HasActed();
	}

	bool CanMovePlayerUnit(const ALFPTacticsUnit* Unit, const ALFPTurnManager* TurnManager)
	{
		return CanControlPlayerUnit(Unit, TurnManager) &&
			Unit->GetCurrentMovePoints() > 0 &&
			TurnManager->HasEnoughFactionAP(Unit->GetAffiliation(), MovementActionPointCost);
	}
}

ALFPTacticsPlayerController::ALFPTacticsPlayerController()
{
	TargetUnit = nullptr;
	SelectedUnit = nullptr;
	SelectedTile = nullptr;
	bIsSelecting = false;
	CameraRotationPitchAngle = 60.0f;
	CameraRotationYawAngle = 0.0f;
	bDebugEnabled = false;
	CameraOffset = FVector::ZeroVector;
	bIsDragging = false;
	CurrentControlState = EPlayControlState::MoveState;
	LastControlState = CurrentControlState;
	BattleCardComponent = CreateDefaultSubobject<ULFPBattleCardComponent>(TEXT("BattleCardComponent"));

	// 创建地图编辑器组件
	MapEditorComponent = CreateDefaultSubobject<ULFPMapEditorComponent>(TEXT("MapEditorComponent"));
}

void ALFPTacticsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 设置输入模式
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	// 获取网格管理器
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		GridManager = Cast<ALFPHexGridManager>(FoundActors[0]);
	}

	// 设置Enhanced Input
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultInputMapping)
		{
			Subsystem->AddMappingContext(DefaultInputMapping, 0);
		}
	}

	// 初始化战斗 HUD
	if (BattleHUDClass)
	{
		BattleHUDWidget = CreateWidget<ULFPBattleHUDWidget>(this, BattleHUDClass);
		if (BattleHUDWidget)
		{
			BattleHUDWidget->AddToViewport();

			// 初始化回合速度列表（HUD 内始终可见的子组件）
			if (ULFPTurnSpeedListWidget* TurnSpeedList = BattleHUDWidget->GetTurnSpeedListWidget())
			{
				TurnSpeedList->InitializeTurnOrder();
			}

			ALFPTurnManager* TurnManager = GetTurnManager();
			BattleHUDWidget->InitializeEnergyBar(TurnManager);
			BattleHUDWidget->InitializeCurrentUnitInfo(TurnManager);
		}
	}

	// 绑定阶段变化委托
	if (ALFPTurnManager* TurnManager = GetTurnManager())
	{
		TurnManager->OnPhaseChanged.AddDynamic(this, &ALFPTacticsPlayerController::OnPhaseChanged);

		// StartGame() 在 GameMode::StartPlay() 中执行，早于 PlayerController::BeginPlay()
		// 此时阶段已设置但未广播，需主动检查并触发布置阶段
		if (TurnManager->GetCurrentPhase() == EBattlePhase::BP_Deployment)
		{
			OnDeploymentPhaseStarted();
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (ULFPBetrayalWorldSubsystem* BetrayalSubsystem = World->GetSubsystem<ULFPBetrayalWorldSubsystem>())
		{
			BetrayalSubsystem->OnBetrayalResolvedInSubsystem.AddUniqueDynamic(this, &ALFPTacticsPlayerController::OnUnitBetrayedToPlayer);
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
}

void ALFPTacticsPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULFPBetrayalWorldSubsystem* BetrayalSubsystem = World->GetSubsystem<ULFPBetrayalWorldSubsystem>())
		{
			BetrayalSubsystem->OnBetrayalResolvedInSubsystem.RemoveDynamic(this, &ALFPTacticsPlayerController::OnUnitBetrayedToPlayer);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ALFPTacticsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 绑定Enhanced Input组件
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 左键主操作：选择 + 即时确认
		EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnPrimaryActionStarted);
		EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnPrimaryActionCompleted);

		// 其他操作
		EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnCancelAction);
		//EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnRotateCamera);
		EnhancedInputComponent->BindAction(DebugToggleAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnToggleDebug);
		EnhancedInputComponent->BindAction(SkipTurnAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnSkipTurnAction);

		// 相机操作
		EnhancedInputComponent->BindAction(CameraPanAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCameraPan);
		EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnCameraDragStarted);
		EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCameraDragTriggered);
		EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnCameraDragCompleted);
		EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCameraZoom);

		// 地图编辑器切换
		EnhancedInputComponent->BindAction(ToggleEditorAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnToggleEditorAction);
	}
}

void ALFPTacticsPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDragging)
	{
		DragTime += DeltaTime;
	}

	UpdateActiveCardDragVisual();


	// 更新相机位置
	if (GridManager)
	{
		FVector CameraLocation = GridManager->GetActorLocation() + CameraOffset;
		FRotator CameraRotation(CameraRotationPitchAngle, CameraRotationYawAngle, 0);
		SetControlRotation(CameraRotation);

		// 计算相机位置偏移
		FVector DirectionOffset = CameraRotation.Vector() * CurrentZoom;
		DirectionOffset.Z = CurrentZoom * 0.5f; // 高度由缩放比例决定

		// 设置相机位置
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn)
		{
			FVector TargetLocation = CameraLocation + DirectionOffset;
			if (bSnapCameraNextFrame)
			{
				// 首帧直接跳到目标位置，避免切场景后相机大幅滑动
				ControlledPawn->SetActorLocation(TargetLocation);
				bSnapCameraNextFrame = false;
			}
			else
			{
				ControlledPawn->SetActorLocation(FMath::VInterpTo(
					ControlledPawn->GetActorLocation(),
					TargetLocation,
					DeltaTime,
					5.0f // 插值速度
				));
			}
		}

		// 获取鼠标位置
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY))
		{
			FVector2D ScreenPosition(MouseX, MouseY);
			// 获取光标下的六边形Tile
			ALFPHexTile* HoveredTile = GridManager->GetHexTileUnderCursor(ScreenPosition, this);
			// 处理鼠标悬停逻辑
			if (HoveredTile != LastHoveredTile)
			{
				if (LastHoveredTile)
				{
					//LastHoveredTile->OnMouseExit();
				}

				if (HoveredTile)
				{
					SelectTile(HoveredTile);
					//HoveredTile->OnMouseEnter();
				}

				LastHoveredTile = HoveredTile;
			}

			// 敌人计划预览：检测悬停的格子上是否有带行动计划的敌人
			if (CachedBattlePhase == EBattlePhase::BP_EnemyPlanning || CachedBattlePhase == EBattlePhase::BP_PlayerActionPhase || CachedBattlePhase == EBattlePhase::BP_EnemyActionPhase)
			{
				if (IsInInspectionMode())
				{
					if (PreviewedEnemy)
					{
						HideEnemyPlanPreview();
					}
				}
				else
				{
					ALFPTacticsUnit* HoveredEnemy = nullptr;
					if (LastHoveredTile)
					{
						ALFPTacticsUnit* UnitOnTile = LastHoveredTile->GetUnitOnTile();
						if (UnitOnTile && UnitOnTile->IsEnemy() && UnitOnTile->HasActionPlan())
						{
							HoveredEnemy = UnitOnTile;
						}
					}

					if (HoveredEnemy && HoveredEnemy != PreviewedEnemy && CurrentControlState == EPlayControlState::MoveState)
					{
						ShowEnemyPlanPreview(HoveredEnemy);
					}
					else if (!HoveredEnemy && PreviewedEnemy)
					{
						HideEnemyPlanPreview();
					}
				}
			}
		}
	}

	// 更新地图视觉效果
	if (LastControlState != CurrentControlState)
	{
		switch (CurrentControlState)
		{
		case EPlayControlState::MoveState:
			break;
		case EPlayControlState::SkillReleaseState:
			break;
		default:
			break;
		}
	}
}

// 左键主操作开始
void ALFPTacticsPlayerController::OnPrimaryActionStarted(const FInputActionValue& Value)
{
	if (bWaitingForMove) return;

	bIsSelecting = true;
	bPrimaryActionStartedOverUI = IsPrimaryActionOverUI();

	// 获取鼠标位置
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		SelectionStart = FVector2D(MouseX, MouseY);
	}
}

// 左键主操作完成：选择 + 即时确认
void ALFPTacticsPlayerController::OnPrimaryActionCompleted(const FInputActionValue& Value)
{
	if (bWaitingForMove)
	{
		bIsSelecting = false;
		bPrimaryActionStartedOverUI = false;
		return;
	}

	bIsDragging = false;
	if (DragTime > DragThresholdTime)
	{
		DragTime = 0.f;
		bIsSelecting = false;
		bPrimaryActionStartedOverUI = false;
		return;
	}
	DragTime = 0.f;

	if (!bIsSelecting)
	{
		bPrimaryActionStartedOverUI = false;
		return;
	}
	bIsSelecting = false;

	// 获取鼠标位置
	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY))
	{
		bPrimaryActionStartedOverUI = false;
		return;
	}

	const FVector2D SelectionEnd(MouseX, MouseY);
	const bool bStartedOverUI = bPrimaryActionStartedOverUI;
	bPrimaryActionStartedOverUI = false;

	if (!bHasActiveDraggedCard && (bStartedOverUI || IsPrimaryActionOverUI()))
	{
		return;
	}

	// 简单的点击检测（非拖拽）
	if (FVector2D::Distance(SelectionStart, SelectionEnd) >= 10.0f)
	{
		return;
	}

	if (TryCompleteActiveCardDragAtViewportPosition(SelectionEnd))
	{
		return;
	}

	FHitResult HitResult;
	GetHitResultAtScreenPosition(SelectionEnd, ECC_Visibility, false, HitResult);
	HandlePrimaryActionAtHit(HitResult);
}

void ALFPTacticsPlayerController::OnSelectStarted(const FInputActionValue& Value)
{
	OnPrimaryActionStarted(Value);
}

void ALFPTacticsPlayerController::OnSelectCompleted(const FInputActionValue& Value)
{
	OnPrimaryActionCompleted(Value);
}

void ALFPTacticsPlayerController::OnConfirmAction(const FInputActionValue& Value)
{
	if (!bIsSelecting)
	{
		bIsSelecting = true;
		bPrimaryActionStartedOverUI = false;
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY))
		{
			SelectionStart = FVector2D(MouseX, MouseY);
		}
	}

	OnPrimaryActionCompleted(Value);
}

bool ALFPTacticsPlayerController::IsPrimaryActionOverUI() const
{
	if (!BattleHUDWidget)
	{
		return false;
	}

	return IsVisibleAndHovered(BattleHUDWidget->GetSkillSelectionWidget()) ||
		IsVisibleAndHovered(BattleHUDWidget->GetDeploymentWidget()) ||
		IsVisibleAndHovered(BattleHUDWidget->GetBattleResultWidget()) ||
		IsVisibleAndHovered(BattleHUDWidget->GetCurrentUnitInfoWidget()) ||
		IsVisibleAndHovered(BattleHUDWidget->GetCardHandWidget()) ||
		IsVisibleAndHovered(BattleHUDWidget->GetTurnSpeedListWidget());
}

bool ALFPTacticsPlayerController::IsInInspectionMode() const
{
	return bIsInspectingUnit;
}

void ALFPTacticsPlayerController::ClearSelectionHighlight()
{
	if (HighlightedSelectionTile)
	{
		HighlightedSelectionTile->ShowSelectionHighlight(false);
		HighlightedSelectionTile = nullptr;
	}
}

void ALFPTacticsPlayerController::UpdateSelectionHighlightForUnit(ALFPTacticsUnit* Unit)
{
	ALFPHexTile* UnitTile = Unit ? Unit->GetCurrentTile() : nullptr;
	if (HighlightedSelectionTile && HighlightedSelectionTile != UnitTile)
	{
		HighlightedSelectionTile->ShowSelectionHighlight(false);
	}

	HighlightedSelectionTile = UnitTile;
	if (HighlightedSelectionTile)
	{
		HighlightedSelectionTile->ShowSelectionHighlight(true);
	}
}

bool ALFPTacticsPlayerController::ExitInspectionMode()
{
	if (!bIsInspectingUnit)
	{
		return false;
	}

	bIsInspectingUnit = false;

	if (SelectedUnit)
	{
		SelectedUnit->SetSelected(false);
		SelectedUnit = nullptr;
	}
	ClearSelectionHighlight();

	if (BattleHUDWidget)
	{
		BattleHUDWidget->ExitInspectionMode(this);
		BattleHUDWidget->HideSkillSelection();
		BattleHUDWidget->SetCurrentUnitInfoUnit(nullptr);
		BattleHUDWidget->ResetCardHandUnitPlayablePopups();
	}

	ClearMovementAndRange();
	return true;
}

bool ALFPTacticsPlayerController::HandleInspectionPrimaryAction(const FHitResult& HitResult)
{
	if (!IsInInspectionMode())
	{
		return false;
	}

	ALFPTacticsUnit* ClickedUnit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
	if (!ClickedUnit && LastHoveredTile)
	{
		ClickedUnit = LastHoveredTile->GetUnitOnTile();
	}

	if (ClickedUnit && ClickedUnit != SelectedUnit)
	{
		SelectUnit(ClickedUnit);
		return true;
	}

	ExitInspectionMode();
	return true;
}

void ALFPTacticsPlayerController::HandlePrimaryActionAtHit(const FHitResult& HitResult)
{
	if (HandleInspectionPrimaryAction(HitResult))
	{
		return;
	}

	// 布置阶段优先处理
	if (bIsInDeployment)
	{
		// 检测是否点击了已部署的单位
		ALFPTacticsUnit* ClickedUnit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
		if (ClickedUnit && UnitToPartyIndex.Contains(ClickedUnit))
		{
			int32 ClickedPartyIndex = UnitToPartyIndex[ClickedUnit];

			// 已有出战单位选中且点击不同单位 → 交换位置
			if (SelectedMapUnitIndex >= 0 && SelectedMapUnitIndex != ClickedPartyIndex)
			{
				SwapDeployedUnits(SelectedMapUnitIndex, ClickedPartyIndex);
				ClearDeploymentSelection();
				return;
			}

			SelectDeployedUnit(ClickedUnit);
			return;
		}

		// 检测是否点击了部署格子
		if (LastHoveredTile)
		{
			OnDeploymentTileClicked(LastHoveredTile);
			return;
		}

		// 点击空白区域：清除选中
		ClearDeploymentSelection();
		return;
	}

	// 编辑器模式优先处理
	if (MapEditorComponent && MapEditorComponent->IsEditorActive())
	{
		if (LastHoveredTile)
		{
			MapEditorComponent->ApplyToolToTile(LastHoveredTile);
		}
		else if (GridManager && MapEditorComponent->GetCurrentTool() == ELFPMapEditorTool::MET_AddTile)
		{
			// 对空白区域添加格子：射线投射到 Z=0 平面计算坐标
			FVector WorldOrigin, WorldDirection;
			DeprojectMousePositionToWorld(WorldOrigin, WorldDirection);
			if (FMath::Abs(WorldDirection.Z) > KINDA_SMALL_NUMBER)
			{
				float T = -WorldOrigin.Z / WorldDirection.Z;
				FVector HitPoint = WorldOrigin + WorldDirection * T;
				FVector2D LocalLoc(
					HitPoint.X - GridManager->GetActorLocation().X,
					HitPoint.Y - GridManager->GetActorLocation().Y
				);
				FLFPHexCoordinates Coord = FLFPHexCoordinates::FromWorldLocation(
					LocalLoc, GridManager->GetHexSize(), GridManager->GetVerticalScale());
				MapEditorComponent->ApplyToolToCoord(Coord.Q, Coord.R);
			}
		}
		return;
	}

	if (ALFPTacticsUnit* ClickedUnit = Cast<ALFPTacticsUnit>(HitResult.GetActor()))
	{
		if (CurrentControlState == EPlayControlState::SkillReleaseState && bIsReleaseSkill && CurrentSelectedSkill)
		{
			HandlePrimaryTileClicked(ClickedUnit->GetCurrentTile());
			return;
		}

		SelectUnit(ClickedUnit);
		return;
	}

	if (LastHoveredTile)
	{
		HandlePrimaryTileClicked(LastHoveredTile);
	}
}

void ALFPTacticsPlayerController::HandlePrimaryTileClicked(ALFPHexTile* Tile)
{
	if (!Tile)
	{
		return;
	}

	if (!SelectedUnit || !GridManager)
	{
		if (ALFPTacticsUnit* UnitOnTile = Tile->GetUnitOnTile())
		{
			SelectUnit(UnitOnTile);
		}
		return;
	}

	switch (CurrentControlState)
	{
	case EPlayControlState::MoveState:
		if (ALFPTacticsUnit* UnitOnTile = Tile->GetUnitOnTile())
		{
			SelectUnit(UnitOnTile);
			return;
		}

		if (SelectedTile == Tile && MovementRangeTiles.Contains(Tile) && Tile->IsWalkable() && !Tile->IsOccupied())
		{
			ConfirmMove();
		}
		break;

	case EPlayControlState::SkillReleaseState:
		if (SelectedTile == Tile && IsCurrentSkillTargetTileValid(Tile))
		{
			ExecuteSkill(CurrentSelectedSkill);
		}
		break;

	default:
		break;
	}
}

bool ALFPTacticsPlayerController::IsCurrentSkillTargetTileValid(ALFPHexTile* Tile) const
{
	if (!Tile || !CurrentSelectedSkill)
	{
		return false;
	}

	const FLFPHexCoordinates SelectedCoord = Tile->GetCoordinates();
	const TArray<FLFPHexCoordinates> ReleaseRangeCoords = CurrentSelectedSkill->GetReleaseRangeInGrid();
	return ReleaseRangeCoords.Contains(SelectedCoord) &&
		CurrentSelectedSkill->IsValidReleaseTargetTile(Tile);
}

void ALFPTacticsPlayerController::OnCancelAction(const FInputActionValue& Value)
{
	if (bWaitingForMove) return;
	bIsDragging = false;
	if (DragTime > DragThresholdTime)
	{
		DragTime = 0.f;
		return;
	}
	DragTime = 0.f;

	// 检查模式优先取消：如果正在检查其他单位，右键还原到回合单位
	if (bHasActiveDraggedCard)
	{
		CancelActiveCardDrag();
		return;
	}

	if (ExitInspectionMode())
	{
		return;
	}

	// 布置阶段：清除选中
	if (bIsInDeployment)
	{
		ClearDeploymentSelection();
		return;
	}

	// 技能释放状态：取消技能选择，回到移动预览状态
	if (bIsReleaseSkill)
	{
		CancelCardTargetSelection();
		ShowUnitRange(EUnitRange::UR_Move);
		return;
	}

	// Cancel tile/path preview first, then clear unit selection on the next cancel.
	const bool bHadSelectedTileOrPath = SelectedTile != nullptr || !CurrentPath.IsEmpty();
	if (bHadSelectedTileOrPath)
	{
		HidePathToRange();
		SelectUnit(nullptr);
		return;
	}

	if (SelectedUnit)
	{
		SelectUnit(nullptr);
		return;
	}

	HidePathToRange();
}

//void ALFPTacticsPlayerController::OnRotateCamera(const FInputActionValue& Value)
//{
//    // 获取输入值
//    const FVector2D RotationValue = Value.Get<FVector2D>();
//    if (FMath::Abs(RotationValue.X) > 0.1f)
//    {
//        CameraRotationYawAngle += RotationValue.X * 2.0f; // 旋转速度
//    }
//}

void ALFPTacticsPlayerController::OnToggleDebug(const FInputActionValue& Value)
{
	ToggleDebugDisplay();
}

void ALFPTacticsPlayerController::OnSkipTurnAction(const FInputActionValue& Value)
{
	if (bWaitingForMove) return;

	ALFPTurnManager* TM = GetTurnManager();
	if (TM && TM->GetCurrentPhase() == EBattlePhase::BP_PlayerActionPhase)
	{
		// 玩家行动阶段：无选中单位或已行动单位时，触发 End Player Phase
		if (!SelectedUnit || SelectedUnit->HasActed())
		{
			EndPlayerTurn();
			return;
		}
	}

	if (!SelectedUnit) return;
	SkipTurn(SelectedUnit);
}

void ALFPTacticsPlayerController::OnCameraPan(const FInputActionValue& Value)
{
	// 获取输入值（WASD/方向键）
	const FVector2D PanValue = Value.Get<FVector2D>();

	// 计算移动方向（基于当前相机旋转）
	FRotator CameraRotation(CameraRotationPitchAngle, CameraRotationYawAngle, 0);
	FVector Forward = CameraRotation.RotateVector(FVector::ForwardVector);
	FVector Right = CameraRotation.RotateVector(FVector::RightVector);

	// 计算移动偏移
	FVector PanDirection = (Forward * PanValue.Y) + (Right * PanValue.X);
	PanDirection.Z = 0;
	PanDirection.Normalize();

	// 应用移动
	CameraOffset += PanDirection * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
}

void ALFPTacticsPlayerController::OnCameraDragStarted(const FInputActionValue& Value)
{
	// 记录拖拽开始位置
	bIsDragging = true;
	GetMousePosition(DragStartPosition.X, DragStartPosition.Y);
}

void ALFPTacticsPlayerController::OnCameraDragTriggered(const FInputActionValue& Value)
{
	if (!bIsDragging) return;

	// 获取当前鼠标位置
	FVector2D CurrentMousePosition;
	GetMousePosition(CurrentMousePosition.X, CurrentMousePosition.Y);

	// 计算鼠标移动偏移
	FVector2D MouseDelta = CurrentMousePosition - DragStartPosition;

	// 反转Y轴（屏幕Y轴向下，世界Y轴向上）
	MouseDelta.Y = -MouseDelta.Y;

	// 计算移动方向（基于当前相机旋转）
	FRotator CameraRotation(0, CameraRotationYawAngle, 0);
	FVector Right = CameraRotation.RotateVector(FVector::RightVector);
	FVector Forward = CameraRotation.RotateVector(FVector::ForwardVector);

	// 计算移动偏移
	FVector PanDirection = (Right * -MouseDelta.X) + (Forward * -MouseDelta.Y);
	//PanDirection.Normalize();

	// 应用移动
	CameraOffset += PanDirection * CameraDragSpeed;

	// 更新起始位置为当前位置
	DragStartPosition = CurrentMousePosition;
}

void ALFPTacticsPlayerController::OnCameraDragCompleted(const FInputActionValue& Value)
{
	//bIsDragging = false;
}

void ALFPTacticsPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	const float ZoomValue = Value.Get<float>();
	CurrentZoom = FMath::Clamp(CurrentZoom - ZoomValue * CameraZoomSpeed, MinZoomDistance, MaxZoomDistance);
}

void ALFPTacticsPlayerController::SelectUnit(ALFPTacticsUnit* Unit)
{
	// 取消之前选中的单位
	if (SelectedUnit && SelectedUnit != Unit)
	{
		SelectedUnit->SetSelected(false);
		ClearMovementAndRange();
	}

	if (SelectedUnit != Unit)
	{
		SelectedTile = nullptr;
		CurrentSelectedSkill = nullptr;
		bIsReleaseSkill = false;
		CurrentControlState = EPlayControlState::MoveState;
		if (GridManager)
		{
			GridManager->ClearRangeHighlight(EUnitRange::UR_SkillRelease);
			GridManager->ClearRangeHighlight(EUnitRange::UR_SkillEffect);
		}
		if (BattleHUDWidget)
		{
			BattleHUDWidget->ClearSelectedSkill();
		}
	}

	// 选择新单位
	SelectedUnit = Unit;
	if (!SelectedUnit)
	{
		bIsInspectingUnit = false;
		ClearSelectionHighlight();
		ClearMovementAndRange();
		if (BattleHUDWidget)
		{
			BattleHUDWidget->HideSkillSelection();
			BattleHUDWidget->SetCurrentUnitInfoUnit(nullptr);
		}
		UpdateCardHandPlayablePopupsForSelection();
		return;
	}

	ALFPTurnManager* TM = GetTurnManager();
	const bool bCanControlSelectedUnit = CanControlPlayerUnit(SelectedUnit, TM);
	bIsInspectingUnit = SelectedUnit->IsEnemy();

	SelectedUnit->SetSelected(true);
	UpdateSelectionHighlightForUnit(SelectedUnit);

	if (bIsInspectingUnit)
	{
		if (PreviewedEnemy)
		{
			HideEnemyPlanPreview();
		}

		if (BattleHUDWidget)
		{
			BattleHUDWidget->EnterInspectionMode(SelectedUnit, this);
		}
		ClearMovementAndRange();
		UpdateCardHandPlayablePopupsForSelection();
		return;
	}

	if (BattleHUDWidget)
	{
		BattleHUDWidget->ExitInspectionMode(this);
		BattleHUDWidget->SetCurrentUnitInfoUnit(SelectedUnit);
		BattleHUDWidget->ShowCurrentUnitInfo();
	}

	if (bCanControlSelectedUnit)
	{
		ShowUnitRange(EUnitRange::UR_Move);
		HandleSkillSelection();
		UpdateCardHandPlayablePopupsForSelection();
	}
	else
	{
		ClearMovementAndRange();
		HideSkillSelection();
		UpdateCardHandPlayablePopupsForSelection();
	}
}

void ALFPTacticsPlayerController::SelectTile(ALFPHexTile* Tile)
{
	if (!SelectedUnit || !GridManager) return;

	switch (CurrentControlState)
	{
	case EPlayControlState::MoveState:
		// 检查是否在可移动范围内
		if (MovementRangeTiles.Contains(Tile))
		{
			SelectedTile = Tile;
			ShowPathToSelectedTile();
		}
		else
		{
			SelectedTile = Tile;
			// 清除之前高亮显示的路径
			HidePathToRange();
		}
		break;
	case EPlayControlState::SkillReleaseState:
		SelectedTile = Tile;
		if (CurrentSelectedSkill)
		{
			FLFPHexCoordinates SelectedCoord = Tile->GetCoordinates();
			TArray<FLFPHexCoordinates> ReleaseRangeCoords = CurrentSelectedSkill->GetReleaseRangeInGrid();
			if (ReleaseRangeCoords.Contains(SelectedCoord) && CurrentSelectedSkill->IsValidReleaseTargetTile(Tile))
			{
				TArray<FLFPHexCoordinates> EffectRangeCoords = CurrentSelectedSkill->GetEffectRange();
				TArray<FLFPHexCoordinates> EffectRangeInGridCoords;
				for (FLFPHexCoordinates Coord : EffectRangeCoords)
				{
					FLFPHexCoordinates CoordInGrid = FLFPHexCoordinates();
					CoordInGrid.Q = SelectedCoord.Q + Coord.Q;
					CoordInGrid.R = SelectedCoord.R + Coord.R;
					CoordInGrid.S = SelectedCoord.S + Coord.S;
					EffectRangeInGridCoords.Add(CoordInGrid);
				}
				if (GridManager)
				{
					GridManager->ShowRangeHighlightByCoords(EffectRangeInGridCoords, EUnitRange::UR_SkillEffect);
				}
			}
			else
			{
				GridManager->ClearRangeHighlight(EUnitRange::UR_SkillEffect);
			}
		}
		break;
	default:
		break;
	}
}

void ALFPTacticsPlayerController::ConfirmMove()
{
	if (!SelectedUnit || !SelectedTile)
	{
		return;
	}
	ALFPTurnManager* TM = GetTurnManager();
	if (!CanMovePlayerUnit(SelectedUnit, TM))
	{
		return;
	}
	if (SelectedTile->IsOccupied() || !SelectedTile->IsWalkable())
	{
		return;
	}
	// 检查目标格是否在当前移动范围内
	if (!MovementRangeTiles.Contains(SelectedTile))
	{
		return;
	}
	// 约束寻路验证：在当前移动范围内查找路径（找不到合法路径则阻止移动）
	if (GridManager)
	{
		ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
		if (UnitTile)
		{
			TArray<ALFPHexTile*> ConstrainedPath = GridManager->FindPath(UnitTile, SelectedTile, &MovementRangeTiles, SelectedUnit->GetAffiliation());
			if (ConstrainedPath.Num() == 0)
			{
				return;
			}
		}
	}
	// 移动开始后先清除路径高亮；移动完成时立即提交位置并扣除 AP。
	HidePathToRange();

	// 将移动范围约束写入单位，MoveToTile 将在此范围内寻路
	SelectedUnit->MovementRangeTiles = MovementRangeTiles;

	MoveUnit(SelectedUnit, SelectedTile);

	CurrentPath.Empty();
}

void ALFPTacticsPlayerController::ShowUnitRange(EUnitRange UnitRange)
{
	if (!SelectedUnit || !GridManager) return;

	GridManager->ClearAllHighlights();
	CacheRangeTiles.Empty();
	MovementRangeTiles.Empty();

	switch (UnitRange)
	{
	case EUnitRange::UR_Default:
		break;
	case EUnitRange::UR_Move:
	{
		if (!CanMovePlayerUnit(SelectedUnit, GetTurnManager()))
		{
			break;
		}

		ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
		if (UnitTile)
		{
			CacheRangeTiles = MovementRangeTiles = GridManager->GetTilesInRange(UnitTile, SelectedUnit->GetMovementRange(), SelectedUnit->GetAffiliation());
			// GetTilesInRange 排除了中心格，手动纳入原点
			MovementRangeTiles.AddUnique(UnitTile);
			CacheRangeTiles.AddUnique(UnitTile);
			// 显示范围高亮（描边 + 填充）
			GridManager->ShowRangeHighlight(MovementRangeTiles, EUnitRange::UR_Move);
		}
		break;
	}
	default:
		break;
	}
}

void ALFPTacticsPlayerController::ShowPathToSelectedTile()
{
	if (!SelectedUnit || !SelectedTile || !GridManager) return;

	// 清除之前显示的路径
	HidePathToRange();
	// 寻找路径
	ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
	if (UnitTile)
	{
		// 约束寻路：有移动范围时限制在范围内，保证显示路径与实际路径一致
		if (MovementRangeTiles.Num() > 0)
		{
			CurrentPath = GridManager->FindPath(UnitTile, SelectedTile, &MovementRangeTiles, SelectedUnit->GetAffiliation());
		}
		else
		{
			CurrentPath = GridManager->FindPath(UnitTile, SelectedTile, nullptr, SelectedUnit->GetAffiliation());
		}

		// 高亮显示路径
		GridManager->ShowPathHighlight(CurrentPath);
	}
}

void ALFPTacticsPlayerController::HidePathToDefault()
{
	if (GridManager)
	{
		GridManager->ClearPathHighlight();
		GridManager->ClearAllHighlights();
	}
	CurrentPath.Empty();
}

void ALFPTacticsPlayerController::HidePathToRange()
{
	if (GridManager)
	{
		GridManager->ClearPathHighlight();
	}
	CurrentPath.Empty();
}

void ALFPTacticsPlayerController::ToggleDebugDisplay()
{
	bDebugEnabled = !bDebugEnabled;
	if (GridManager)
	{
		GridManager->SetDebugEnabled(bDebugEnabled);
	}
}

void ALFPTacticsPlayerController::MoveUnit(ALFPTacticsUnit* Unit, ALFPHexTile* TargetTile)
{
	if (!Unit || !TargetTile || !Unit->CanAct()) return;
	if (!CanMovePlayerUnit(Unit, GetTurnManager())) return;

	// 执行移动（启动移动动画）
	if (Unit->MoveToTile(TargetTile))
	{
		// 锁定输入，等待移动完成
		bWaitingForMove = true;
		MovingUnit = Unit;
		Unit->OnMoveFinished.AddDynamic(this, &ALFPTacticsPlayerController::OnUnitMoveComplete);
	}
}

void ALFPTacticsPlayerController::OnUnitMoveComplete()
{
	bWaitingForMove = false;

	ALFPTacticsUnit* CompletedUnit = MovingUnit;

	// 解绑委托
	if (CompletedUnit)
	{
		CompletedUnit->OnMoveFinished.RemoveDynamic(this, &ALFPTacticsPlayerController::OnUnitMoveComplete);
		CompletedUnit->CommitMovePosition();
		CompletedUnit->ConsumeActionPoints(MovementActionPointCost);
		CompletedUnit->MovementRangeTiles.Empty();
	}

	if (SelectedUnit && SelectedUnit == CompletedUnit)
	{
		SelectedTile = nullptr;
		ClearMovementAndRange();
		UpdateSelectionHighlightForUnit(SelectedUnit);
		if (BattleHUDWidget)
		{
			BattleHUDWidget->SetCurrentUnitInfoUnit(SelectedUnit);
		}
		HandleSkillSelection();
	}

	MovingUnit = nullptr;
}

void ALFPTacticsPlayerController::SkipTurn(ALFPTacticsUnit* Unit)
{

	if (!Unit || !Unit->CanAct()) return;

	// 兜底提交遗留的未确认移动；正常移动会在移动动画完成时提交。
	Unit->CommitMovePosition();

	// 可选：消耗1个行动力作为跳过代价
	// Unit->ConsumeMovePoints(1);
	CurrentControlState = EPlayControlState::MoveState;
	// 通知回合管理器单位完成行动
	if (ALFPTurnManager* TurnManager = GetTurnManager())
	{
		TurnManager->OnUnitFinishedAction(Unit);
	}
}

void ALFPTacticsPlayerController::ExecuteSkill(ULFPSkillBase* CurrentSkill)
{
	if (SelectedUnit && CurrentSkill)
	{
		if (CurrentSkill->TargetType == ESkillTargetType::Self)
		{
			bIsSelecting = false;
			bPrimaryActionStartedOverUI = true;
		}

		// 兜底提交遗留的未确认移动；正常移动会在移动动画完成时提交。
		SelectedUnit->CommitMovePosition();
		ALFPHexTile* TargetTile = SelectedTile;
		if (CurrentSkill->TargetType == ESkillTargetType::Self)
		{
			TargetTile = SelectedUnit->GetCurrentTile();
		}

		if (SelectedUnit->ExecuteSkill(CurrentSkill, TargetTile))
		{
			FinishCardForSkill(CurrentSkill);
			if (BattleHUDWidget)
			{
				BattleHUDWidget->RefreshCardHand();
			}
			bIsReleaseSkill = false;
			SelectedUnit->ConsumeActionPoints(CurrentSkill->ActionPointCost);
			CurrentSelectedSkill = nullptr;
			SelectedTile = nullptr;
			CurrentControlState = EPlayControlState::MoveState;
			ShowUnitRange(EUnitRange::UR_Move);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("玩家主动释放失败: 技能[%s]"), *CurrentSkill->SkillName.ToString());
		}
	}
}

void ALFPTacticsPlayerController::EndPlayerTurn()
{
	ALFPTurnManager* TM = GetTurnManager();
	if (!TM || TM->GetCurrentPhase() != EBattlePhase::BP_PlayerActionPhase) return;

	// 清除当前选择/状态
	if (bIsReleaseSkill)
	{
		CancelCardTargetSelection();
	}
	if (BattleHUDWidget)
	{
		BattleHUDWidget->HideSkillSelection();
		BattleHUDWidget->ClearSelectedSkill();
		BattleHUDWidget->ResetCardHandPopups();
	}
	ClearMovementAndRange();
	ClearSelectionHighlight();
	if (SelectedUnit)
	{
		SelectedUnit->SetSelected(false);
	}
	SelectedUnit = nullptr;

	TM->EndPlayerPhase();
}

ALFPTurnManager* ALFPTacticsPlayerController::GetTurnManager() const
{
	TArray<AActor*> FoundManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTurnManager::StaticClass(), FoundManagers);

	if (FoundManagers.Num() > 0)
	{
		return Cast<ALFPTurnManager>(FoundManagers[0]);
	}
	return nullptr;
}

void ALFPTacticsPlayerController::HandleSkillSelection()
{
	if (!SelectedUnit) return;
	if (!BattleHUDWidget) return;

	if (!CanControlPlayerUnit(SelectedUnit, GetTurnManager()))
	{
		BattleHUDWidget->HideSkillSelection();
		return;
	}

	ULFPSkillSelectionWidget* SkillWidget = BattleHUDWidget->GetSkillSelectionWidget();
	if (SkillWidget)
	{
		TArray<ULFPSkillBase*> HandSkills;
		BuildHandSkillListForUnit(SelectedUnit, HandSkills);

		BattleHUDWidget->ShowSkillSelection();
		SkillWidget->InitializeProvidedSkillsInfo(SelectedUnit, this, HandSkills);
	}
}

void ALFPTacticsPlayerController::UpdateCardHandPlayablePopupsForSelection()
{
	if (!BattleHUDWidget)
	{
		return;
	}

	if (CanControlPlayerUnit(SelectedUnit, GetTurnManager()))
	{
		BattleHUDWidget->PopPlayableCardsForUnit(SelectedUnit);
	}
	else
	{
		BattleHUDWidget->ResetCardHandUnitPlayablePopups();
	}
}

void ALFPTacticsPlayerController::ShowActiveCardDragVisual(const FLFPCardInstance& CardInstance,
	TSubclassOf<UUserWidget> DragVisualClass)
{
	ClearActiveCardDragVisual();

	if (!DragVisualClass)
	{
		return;
	}

	ActiveCardDragVisual = CreateWidget<UUserWidget>(this, DragVisualClass);
	if (!ActiveCardDragVisual)
	{
		return;
	}

	if (ULFPCardItemWidget* CardItemVisual = Cast<ULFPCardItemWidget>(ActiveCardDragVisual))
	{
		CardItemVisual->InitializeCardItem(CardInstance, this);
		CardItemVisual->ResetPopupReasons();
	}

	ActiveCardDragVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
	ActiveCardDragVisual->AddToViewport(1000);
	UpdateActiveCardDragVisual();
}

void ALFPTacticsPlayerController::UpdateActiveCardDragVisual()
{
	if (!ActiveCardDragVisual || !ActiveCardDragVisual->IsInViewport())
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D SlatePosition = ViewportScale > KINDA_SMALL_NUMBER
		? FVector2D(MouseX, MouseY) / ViewportScale
		: FVector2D(MouseX, MouseY);
	ActiveCardDragVisual->SetPositionInViewport(SlatePosition + CardClickDragVisualOffset, false);
}

void ALFPTacticsPlayerController::ClearActiveCardDragVisual()
{
	if (ActiveCardDragVisual)
	{
		ActiveCardDragVisual->RemoveFromParent();
		ActiveCardDragVisual = nullptr;
	}
}

bool ALFPTacticsPlayerController::TryCompleteActiveCardDragAtViewportPosition(FVector2D ViewportPosition)
{
	if (!bHasActiveDraggedCard)
	{
		return false;
	}

	const FLFPCardInstance CardToDrop = ActiveDraggedCard;
	const bool bDroppedInNoTargetZone = BattleHUDWidget &&
		BattleHUDWidget->IsCardNoTargetDropPosition(ViewportPosition);

	const bool bShouldEndCardDrag = OnCardDroppedOnViewport(CardToDrop, ViewportPosition, bDroppedInNoTargetZone);
	if (bShouldEndCardDrag)
	{
		EndCardDrag();
	}
	return true;
}

void ALFPTacticsPlayerController::BuildHandSkillListForUnit(ALFPTacticsUnit* Unit, TArray<ULFPSkillBase*>& OutSkills)
{
	OutSkills.Empty();
	HandSkillToCardInstanceID.Empty();

	if (!Unit)
	{
		return;
	}

	if (!BattleCardComponent || !BattleCardComponent->IsInitialized())
	{
		OutSkills = Unit->GetAvailableSkills();
		return;
	}

	// 当前 UI 只展示这名单位能打出的手牌技能；映射保存出牌后要移动的卡牌实例。
	const TArray<FLFPCardInstance> PlayableCards = BattleCardComponent->GetPlayableHandCardsForUnit(Unit);
	for (const FLFPCardInstance& Card : PlayableCards)
	{
		if (!Card.RuntimeSkill)
		{
			continue;
		}

		OutSkills.Add(Card.RuntimeSkill);
		HandSkillToCardInstanceID.Add(Card.RuntimeSkill, Card.InstanceID);
	}
}

bool ALFPTacticsPlayerController::FinishCardForSkill(ULFPSkillBase* Skill)
{
	if (!Skill || !BattleCardComponent)
	{
		return true;
	}

	const int32* CardInstanceID = HandSkillToCardInstanceID.Find(Skill);
	if (!CardInstanceID)
	{
		return true;
	}

	// 技能释放成功后才结算卡牌去向；失败路径不会调用这里，卡仍留在手牌。
	const bool bMoved = BattleCardComponent->FinishPlayingCard(*CardInstanceID);
	HandSkillToCardInstanceID.Remove(Skill);
	return bMoved;
}

void ALFPTacticsPlayerController::CancelCardTargetSelection()
{
	if (CurrentSelectedSkill)
	{
		HandSkillToCardInstanceID.Remove(CurrentSelectedSkill);
	}

	bIsReleaseSkill = false;
	CurrentControlState = EPlayControlState::MoveState;
	CurrentSelectedSkill = nullptr;
	SelectedTile = nullptr;

	if (GridManager)
	{
		GridManager->ClearRangeHighlight(EUnitRange::UR_SkillRelease);
		GridManager->ClearRangeHighlight(EUnitRange::UR_SkillEffect);
	}

	if (BattleHUDWidget)
	{
		BattleHUDWidget->ClearSelectedSkill();
		BattleHUDWidget->RefreshCardHand();
	}
}

bool ALFPTacticsPlayerController::IsNoTargetCard(const FLFPCardInstance& CardInstance) const
{
	return CardInstance.Definition.CardCategory == ELFPCardCategory::NoTarget;
}

bool ALFPTacticsPlayerController::IsDirectEffectCard(const FLFPCardInstance& CardInstance) const
{
	const ULFPSkillBase* Skill = CardInstance.RuntimeSkill;
	if (!Skill)
	{
		return false;
	}

	return Skill->TargetType == ESkillTargetType::AllAlly
		|| Skill->TargetType == ESkillTargetType::AllEnemy
		|| Skill->TargetType == ESkillTargetType::AllUnit;
}

bool ALFPTacticsPlayerController::IsTargetSelectingCard(const FLFPCardInstance& CardInstance) const
{
	return CardInstance.IsValid() &&
		CardInstance.RuntimeSkill &&
		!CardInstance.RuntimeSkill->IsPassiveSkill() &&
		!IsNoTargetCard(CardInstance) &&
		!IsDirectEffectCard(CardInstance);
}

bool ALFPTacticsPlayerController::ExecuteNoTargetCardImmediately(const FLFPCardInstance& CardInstance)
{
	if (!CardInstance.IsValid() || !CardInstance.RuntimeSkill || CardInstance.RuntimeSkill->IsPassiveSkill() ||
		!BattleCardComponent || !IsNoTargetCard(CardInstance))
	{
		return false;
	}

	ULFPSkillBase* RuntimeSkill = CardInstance.RuntimeSkill;
	ALFPTacticsUnit* SourceUnit = CardInstance.SourceUnit.Get();
	RuntimeSkill->Owner = SourceUnit;

	if (RuntimeSkill->HasConfiguredEffects() && !RuntimeSkill->CanExecuteConfiguredEffects(nullptr))
	{
		return false;
	}

	const EUnitAffiliation PaymentFaction = SourceUnit
		? SourceUnit->GetAffiliation()
		: EUnitAffiliation::UA_Player;
	if (CardInstance.Definition.ActionPointCost > 0)
	{
		ALFPTurnManager* TM = GetTurnManager();
		if (!TM || !TM->HasEnoughFactionAP(PaymentFaction, CardInstance.Definition.ActionPointCost))
		{
			return false;
		}

		TM->ConsumeFactionAP(PaymentFaction, CardInstance.Definition.ActionPointCost);
	}

	RuntimeSkill->Execute(nullptr);
	RuntimeSkill->OnSkillUsed();

	const bool bMoved = BattleCardComponent->FinishPlayingCard(CardInstance.InstanceID);
	HandSkillToCardInstanceID.Remove(RuntimeSkill);

	if (BattleHUDWidget)
	{
		BattleHUDWidget->RefreshCardHand();
	}

	return bMoved;
}

bool ALFPTacticsPlayerController::ExecuteDroppedCardImmediately(const FLFPCardInstance& CardInstance, ALFPTacticsUnit* Unit,
	ALFPHexTile* TargetTile)
{
	if (!CardInstance.IsValid() || !CardInstance.RuntimeSkill || CardInstance.RuntimeSkill->IsPassiveSkill() ||
		!Unit || !BattleCardComponent)
	{
		return false;
	}

	if (SelectedUnit != Unit)
	{
		SelectUnit(Unit);
	}

	CardInstance.RuntimeSkill->Owner = Unit;
	CardInstance.RuntimeSkill->UpdateSkillRange();

	if (!Unit->ExecuteSkill(CardInstance.RuntimeSkill, TargetTile))
	{
		return false;
	}

	Unit->ConsumeActionPoints(CardInstance.Definition.ActionPointCost);
	const bool bMoved = BattleCardComponent->FinishPlayingCard(CardInstance.InstanceID);
	HandSkillToCardInstanceID.Remove(CardInstance.RuntimeSkill);

	if (BattleHUDWidget)
	{
		BattleHUDWidget->RefreshCardHand();
	}

	return bMoved;
}

bool ALFPTacticsPlayerController::BeginCardTargetSelection(const FLFPCardInstance& CardInstance,
	ALFPTacticsUnit* Unit)
{
	if (!Unit || !CardInstance.IsValid() || !BattleCardComponent || !IsTargetSelectingCard(CardInstance) ||
		!Unit->CanUseCard(CardInstance))
	{
		return false;
	}

	if (SelectedUnit != Unit)
	{
		SelectUnit(Unit);
	}

	CardInstance.RuntimeSkill->Owner = Unit;
	CardInstance.RuntimeSkill->UpdateSkillRange();
	HandSkillToCardInstanceID.Add(CardInstance.RuntimeSkill, CardInstance.InstanceID);

	HandleSkillTargetSelecting(CardInstance.RuntimeSkill);
	return true;
}

void ALFPTacticsPlayerController::ResolveActiveCardSkillTargetAtViewportPosition(
	const FLFPCardInstance& CardInstance, FVector2D ViewportPosition)
{
	if (!CardInstance.IsValid() || !CurrentSelectedSkill)
	{
		CancelCardTargetSelection();
		return;
	}

	FHitResult HitResult;
	GetHitResultAtScreenPosition(ViewportPosition, ECC_Visibility, false, HitResult);

	ALFPHexTile* TargetTile = nullptr;
	if (ALFPTacticsUnit* HitUnit = Cast<ALFPTacticsUnit>(HitResult.GetActor()))
	{
		TargetTile = HitUnit->GetCurrentTile();
	}
	else if (ALFPHexTile* HitTile = Cast<ALFPHexTile>(HitResult.GetActor()))
	{
		TargetTile = HitTile;
	}
	else
	{
		TargetTile = LastHoveredTile;
	}

	if (TargetTile && IsCurrentSkillTargetTileValid(TargetTile))
	{
		SelectedTile = TargetTile;
		ExecuteSkill(CurrentSelectedSkill);
		return;
	}

	CancelCardTargetSelection();
	if (SelectedUnit)
	{
		ShowUnitRange(EUnitRange::UR_Move);
	}
}


void ALFPTacticsPlayerController::OnHandCardClicked(const FLFPCardInstance& CardInstance,
	TSubclassOf<UUserWidget> DragVisualClass)
{
	if (!CardInstance.RuntimeSkill)
	{
		return;
	}

	ClearCardUsableUnitPreview();

	if (IsTargetSelectingCard(CardInstance))
	{
		if (CanControlPlayerUnit(SelectedUnit, GetTurnManager()))
		{
			if (SelectedUnit->CanUseCard(CardInstance))
			{
				BeginCardDrag(CardInstance, DragVisualClass);
				return;
			}

			if (bIsReleaseSkill)
			{
				CancelCardTargetSelection();
			}
			SelectUnit(nullptr);
		}
	}

	BeginCardDrag(CardInstance, DragVisualClass);
}

void ALFPTacticsPlayerController::PreviewCardUsableUnits(const FLFPCardInstance& CardInstance)
{
	ClearCardUsableUnitPreview();

	if (!CardInstance.IsValid() || !GridManager)
	{
		return;
	}

	if (CardInstance.Definition.CardCategory == ELFPCardCategory::NoTarget)
	{
		return;
	}

	TArray<ALFPHexTile*> CardUsableTiles;
	TArray<ALFPTacticsUnit*> CandidateUnits;
	if (ALFPTurnManager* TurnManager = GetTurnManager())
	{
		for (ALFPTacticsUnit* Unit : TurnManager->GetTurnOrderUnits())
		{
			if (Unit && Unit->GetAffiliation() == EUnitAffiliation::UA_Player)
			{
				CandidateUnits.Add(Unit);
			}
		}
	}

	for (ALFPTacticsUnit* Unit : CandidateUnits)
	{
		if (!Unit || !Unit->IsAlive() || !Unit->CanUseCard(CardInstance))
		{
			continue;
		}

		if (ALFPHexTile* UnitTile = Unit->GetCurrentTile())
		{
			CardUsableTiles.AddUnique(UnitTile);
		}
	}

	GridManager->ShowRangeHighlight(CardUsableTiles, EUnitRange::UR_CardPlayable);
}

void ALFPTacticsPlayerController::ClearCardUsableUnitPreview()
{
	if (GridManager)
	{
		GridManager->ClearRangeHighlight(EUnitRange::UR_CardPlayable);
	}
}

void ALFPTacticsPlayerController::BeginCardDrag(const FLFPCardInstance& CardInstance,
	TSubclassOf<UUserWidget> DragVisualClass, bool bShowDragVisual)
{
	if (!CardInstance.IsValid())
	{
		return;
	}

	if (bHasActiveDraggedCard)
	{
		CancelActiveCardDrag();
	}

	ActiveDraggedCard = CardInstance;
	ActiveCardDragVisualClass = DragVisualClass;
	bHasActiveDraggedCard = true;
	ActiveCardDragPhase = IsTargetSelectingCard(CardInstance)
		? ELFPActiveCardDragPhase::SelectingUsableUnit
		: ELFPActiveCardDragPhase::None;

	if (IsTargetSelectingCard(CardInstance))
	{
		PreviewCardUsableUnits(CardInstance);

		if (CanControlPlayerUnit(SelectedUnit, GetTurnManager()))
		{
			if (SelectedUnit->CanUseCard(CardInstance) &&
				BeginCardTargetSelection(CardInstance, SelectedUnit))
			{
				ActiveCardDragPhase = ELFPActiveCardDragPhase::SelectingSkillTarget;
			}
			else
			{
				SelectUnit(nullptr);
			}
		}
	}

	if (bShowDragVisual)
	{
		ShowActiveCardDragVisual(CardInstance, DragVisualClass);
	}

	if (BattleHUDWidget)
	{
		BattleHUDWidget->SetCardDropTargetActive(true);
	}
}

void ALFPTacticsPlayerController::EndCardDrag()
{
	ActiveDraggedCard = FLFPCardInstance();
	bHasActiveDraggedCard = false;
	ActiveCardDragPhase = ELFPActiveCardDragPhase::None;
	ActiveCardDragVisualClass = nullptr;
	ClearActiveCardDragVisual();
	ClearCardUsableUnitPreview();

	if (BattleHUDWidget)
	{
		BattleHUDWidget->SetCardDropTargetActive(false);
	}

	// 拖拽状态结束后重新应用选中单位的可打出卡牌弹出效果
	UpdateCardHandPlayablePopupsForSelection();
}

void ALFPTacticsPlayerController::CancelActiveCardDrag()
{
	const bool bShouldCancelCardTargetSelection = ActiveCardDragPhase == ELFPActiveCardDragPhase::SelectingSkillTarget;
	if (bShouldCancelCardTargetSelection)
	{
		CancelCardTargetSelection();
		if (SelectedUnit)
		{
			ShowUnitRange(EUnitRange::UR_Move);
		}
	}
	else if (BattleHUDWidget)
	{
		BattleHUDWidget->RefreshCardHand();
	}

	EndCardDrag();
}

bool ALFPTacticsPlayerController::OnCardDroppedOnViewport(const FLFPCardInstance& CardInstance, FVector2D DropScreenPos,
	bool bDroppedInNoTargetZone)
{
	if (!CardInstance.IsValid() || !BattleCardComponent)
	{
		return true;
	}

	if (ActiveCardDragPhase == ELFPActiveCardDragPhase::SelectingSkillTarget)
	{
		ResolveActiveCardSkillTargetAtViewportPosition(CardInstance, DropScreenPos);
		return true;
	}

	// 流程3: 拖到 DropTarget 中配置的无目标释放区域。
	if (bDroppedInNoTargetZone && IsNoTargetCard(CardInstance))
	{
		ExecuteNoTargetCardImmediately(CardInstance);
		return true;
	}

	if (bDroppedInNoTargetZone && IsDirectEffectCard(CardInstance))
	{
		ALFPTacticsUnit* OwnerUnit = CardInstance.SourceUnit.Get();
		if (!OwnerUnit)
		{
			OwnerUnit = SelectedUnit;
		}

		if (OwnerUnit && OwnerUnit->CanUseCard(CardInstance))
		{
			ExecuteDroppedCardImmediately(CardInstance, OwnerUnit, OwnerUnit->GetCurrentTile());
		}
		return true;
	}

	FHitResult HitResult;
	const bool bHit = GetHitResultAtScreenPosition(DropScreenPos, ECC_Visibility, false, HitResult);

	ALFPTacticsUnit* HitUnit = nullptr;
	if (bHit)
	{
		HitUnit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
	}
	if (!HitUnit && LastHoveredTile)
	{
		HitUnit = LastHoveredTile->GetUnitOnTile();
	}

	if (ActiveCardDragPhase == ELFPActiveCardDragPhase::SelectingUsableUnit)
	{
		if (HitUnit && HitUnit->CanUseCard(CardInstance) &&
			BeginCardTargetSelection(CardInstance, HitUnit))
		{
			ActiveCardDragPhase = ELFPActiveCardDragPhase::SelectingSkillTarget;
			if (!ActiveCardDragVisual && ActiveCardDragVisualClass)
			{
				ShowActiveCardDragVisual(CardInstance, ActiveCardDragVisualClass);
			}
			return false;
		}

		return true;
	}

	// 流程1/2: 拖到单位上。
	if (bHit)
	{
		if (HitUnit && HitUnit->CanUseCard(CardInstance))
		{
			ProcessCardDropOnUnit(CardInstance, HitUnit);
		}
	}

	// 无效的释放位置：不做任何操作，原手牌仍保留在手牌区。
	return true;
}

bool ALFPTacticsPlayerController::ProcessCardDropOnUnit(const FLFPCardInstance& CardInstance, ALFPTacticsUnit* Unit)
{
	if (!Unit || !CardInstance.IsValid() || !BattleCardComponent || !CardInstance.RuntimeSkill ||
		CardInstance.RuntimeSkill->IsPassiveSkill() || !Unit->CanUseCard(CardInstance))
	{
		return false;
	}

	if (IsDirectEffectCard(CardInstance))
	{
		return ExecuteDroppedCardImmediately(CardInstance, Unit, Unit->GetCurrentTile());
	}

	// 流程1: 需要目标的卡 → 移到待执行区 → 显示释放范围。
	return BeginCardTargetSelection(CardInstance, Unit);
}

void ALFPTacticsPlayerController::HideSkillSelection()
{
	if (BattleHUDWidget)
	{
		BattleHUDWidget->HideSkillSelection();
	}
}

void ALFPTacticsPlayerController::HideCurrentUnitActionWidgets()
{
	if (BattleHUDWidget)
	{
		BattleHUDWidget->HideSkillSelection();
		BattleHUDWidget->HideCurrentUnitInfo();
	}
	ClearMovementAndRange();
}

void ALFPTacticsPlayerController::ClearMovementAndRange()
{
	if (GridManager)
	{
		GridManager->ClearRangeHighlight(EUnitRange::UR_Move);
	}
	HidePathToRange();
	MovementRangeTiles.Empty();
}

void ALFPTacticsPlayerController::HandleSkillTargetSelecting(ULFPSkillBase* Skill)
{
	if (!SelectedUnit || !Skill) return;
	if (Skill->IsPassiveSkill()) return;

	bIsSelecting = false;
	bPrimaryActionStartedOverUI = true;

	// 获取技能范围内的目标格子
	TArray<FLFPHexCoordinates> TargetTilesCoord = Skill->GetReleaseRangeInGrid();
	if (GridManager)
	{
		bIsReleaseSkill = true;
		CurrentControlState = EPlayControlState::SkillReleaseState;
		GridManager->ClearRangeHighlight(EUnitRange::UR_Move);
		GridManager->ShowRangeHighlightByCoords(TargetTilesCoord, EUnitRange::UR_SkillRelease);
	}

	CurrentSelectedSkill = Skill;
}

//void ALFPTacticsPlayerController::HandleSkillTargetSelected(ALFPHexTile* TargetTile)
//{
//    if (!SelectedUnit || !CurrentSelectedSkill || !TargetTile) return;
//
//    // 执行技能
//    if (SelectedUnit->ExecuteSkill(CurrentSelectedSkill, TargetTile))
//    {
//        //// 技能执行成功
//        //if (GridManager)
//        //{
//        //    GridManager->ClearHighlights();
//        //}
//
//        //// 返回单位选择模式
//        //CurrentSelectionMode = ESelectionMode::UnitSelection;
//        CurrentSelectedSkill = nullptr;
//    }
//}

// ==== 敌人计划预览和阶段感知 ====

void ALFPTacticsPlayerController::ShowEnemyPlanPreview(ALFPTacticsUnit* EnemyUnit)
{
	if (!EnemyUnit || !EnemyUnit->HasActionPlan() || !GridManager) return;
	if (IsInInspectionMode()) return;

	PreviewedEnemy = EnemyUnit;
	const FEnemyActionPlan& Plan = EnemyUnit->CurrentActionPlan;
	PreviewReleaseTiles.Empty();
	PreviewEffectTiles.Empty();

	GridManager->ClearRangeHighlight(EUnitRange::UR_Enemy_SkillRelease);
	GridManager->ClearRangeHighlight(EUnitRange::UR_Enemy_SkillEffect);

	if (!Plan.PlannedSkill)
	{
		return;
	}

	TArray<FLFPHexCoordinates> PreviewReleaseCoords = Plan.PlannedSkill->GetReleaseRangeInGrid();
	for (FLFPHexCoordinates ReleaseCoord : PreviewReleaseCoords)
	{
		if (ALFPHexTile* ReleaseTile = GridManager->GetTileAtCoordinates(ReleaseCoord))
		{
			PreviewReleaseTiles.AddUnique(ReleaseTile);
		}
	}
	// 高亮技能效果范围格子
	PreviewEffectTiles = Plan.EffectAreaTiles;
	GridManager->ClearRangeHighlight(EUnitRange::UR_Move);
	GridManager->ShowRangeHighlight(PreviewReleaseTiles, EUnitRange::UR_Enemy_SkillRelease);
	GridManager->ShowRangeHighlight(PreviewEffectTiles, EUnitRange::UR_Enemy_SkillEffect);
}

void ALFPTacticsPlayerController::HideEnemyPlanPreview()
{
	// 清除预览格子高亮
	if (GridManager)
	{
		PreviewReleaseTiles.Empty();
		PreviewEffectTiles.Empty();
		PreviewedEnemy = nullptr;
		GridManager->ClearRangeHighlight(EUnitRange::UR_Enemy_SkillRelease);
		GridManager->ClearRangeHighlight(EUnitRange::UR_Enemy_SkillEffect);
		ShowUnitRange(EUnitRange::UR_Move);
	}
}

void ALFPTacticsPlayerController::OnPhaseChanged(EBattlePhase NewPhase)
{
	CachedBattlePhase = NewPhase;
	UE_LOG(LogTemp, Log, TEXT("OnPhaseChanged:=%d"), NewPhase);
	// 阶段切换时清理预览状态
	HideEnemyPlanPreview();

	switch (NewPhase)
	{
	case EBattlePhase::BP_Deployment:
		if (BattleHUDWidget)
		{
			BattleHUDWidget->ResetCardHandPopups();
		}
		OnDeploymentPhaseStarted();
		break;

	case EBattlePhase::BP_EnemyPlanning:
		if (BattleHUDWidget)
		{
			BattleHUDWidget->ResetCardHandPopups();
		}
		// 敌人规划阶段：隐藏技能选择UI
		HideSkillSelection();
		break;

	case EBattlePhase::BP_PlayerActionPhase:
		// 玩家行动阶段开始时补到手牌上限；起始手牌已在部署确认后抽出。
		if (BattleCardComponent && BattleCardComponent->IsInitialized())
		{
			BattleCardComponent->DrawUpToHandLimit();
			if (BattleHUDWidget)
			{
				BattleHUDWidget->RefreshCardHand();
			}
		}
		// 玩家行动阶段：BP 侧显示 End Turn 按钮等
		UpdateCardHandPlayablePopupsForSelection();
		break;

	case EBattlePhase::BP_EnemyActionPhase:
		// 敌人行动阶段：禁用玩家输入，隐藏技能UI
		if (bIsReleaseSkill)
		{
			CancelCardTargetSelection();
		}
		HideSkillSelection();
		ClearMovementAndRange();
		ClearSelectionHighlight();
		CurrentControlState = EPlayControlState::MoveState;
		CurrentSelectedSkill = nullptr;
		SelectedUnit = nullptr;
		if (BattleHUDWidget)
		{
			BattleHUDWidget->ResetCardHandPopups();
		}
		break;

	case EBattlePhase::BP_RoundEnd:
		if (BattleHUDWidget)
		{
			BattleHUDWidget->ResetCardHandPopups();
		}
		break;
	}
}

void ALFPTacticsPlayerController::OnUnitBetrayedToPlayer(ALFPTacticsUnit* Unit, EUnitAffiliation OldAffiliation,
	ULFPBetrayalCondition* /*TriggeringCondition*/)
{
	if (!Unit ||
		OldAffiliation == EUnitAffiliation::UA_Player ||
		Unit->GetAffiliation() != EUnitAffiliation::UA_Player)
	{
		return;
	}

	if (BattleCardComponent)
	{
		const int32 AddedCardCount = BattleCardComponent->AddUnitCardsToHand(
			Cast<ULFPGameInstance>(GetGameInstance()), Unit);
		if (AddedCardCount > 0 && BattleHUDWidget)
		{
			BattleHUDWidget->RefreshCardHand();
		}
	}

	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<ALFPTacticsUnit> WeakUnit(Unit);
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakUnit]()
		{
			ALFPTacticsUnit* BetrayedUnit = WeakUnit.Get();
			if (BetrayedUnit &&
				BetrayedUnit->IsAlive() &&
				BetrayedUnit->GetAffiliation() == EUnitAffiliation::UA_Player)
			{
				SelectUnit(BetrayedUnit);
			}
		}));
		return;
	}

	SelectUnit(Unit);
}

void ALFPTacticsPlayerController::OnToggleEditorAction(const FInputActionValue& Value)
{
	if (!MapEditorComponent) return;

	MapEditorComponent->ToggleEditorMode();

	if (MapEditorComponent->IsEditorActive())
	{
		// 创建或显示编辑器 Widget
		if (!MapEditorWidget && MapEditorWidgetClass)
		{
			MapEditorWidget = CreateWidget<ULFPMapEditorWidget>(this, MapEditorWidgetClass);
			if (MapEditorWidget)
			{
				MapEditorWidget->AddToViewport();
				MapEditorWidget->InitializeEditor(MapEditorComponent);
			}
		}
		else if (MapEditorWidget)
		{
			MapEditorWidget->SetVisibility(ESlateVisibility::Visible);
		}

		// 显示鼠标光标
		bShowMouseCursor = true;
	}
	else
	{
		// 隐藏编辑器 Widget
		if (MapEditorWidget)
		{
			MapEditorWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// ============== 布置阶段 ==============

void ALFPTacticsPlayerController::OnDeploymentPhaseStarted()
{
	bIsInDeployment = true;
	SelectedMapUnitIndex = -1;
	ClearSelectionHighlight();

	// 获取玩家出生点
	if (GridManager)
	{
		PlayerSpawnTiles = GridManager->GetSpawnPoints(ELFPSpawnFaction::SF_Player);
		// 按 SpawnIndex 排序
		PlayerSpawnTiles.Sort([](const ALFPHexTile& A, const ALFPHexTile& B)
		{
			return A.SpawnIndex < B.SpawnIndex;
		});

		// 高亮显示出生点
		GridManager->ShowRangeHighlight(PlayerSpawnTiles, EUnitRange::UR_Move);
	}

	// 初始化已部署单位数组
	DeploymentUnitEntries.Empty();
	if (ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance()))
	{
		DeploymentUnitEntries.Append(GI->PartyUnits);
		DeploymentUnitEntries.Append(GI->ReserveUnits);
	}

	DeployedUnits.Empty();
	UnitToPartyIndex.Empty();

	// 自动放置出战单位
	AutoPlacePartyUnits();

	// 显示布置 UI
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (BattleHUDWidget && GI)
	{
		ULFPDeploymentWidget* DeploymentWidget = BattleHUDWidget->GetDeploymentWidget();
		if (DeploymentWidget)
		{
			DeploymentWidget->Setup(GI->PartyUnits, GI->ReserveUnits, GI->UnitRegistry);
			DeploymentWidget->OnUnitClicked.AddDynamic(this, &ALFPTacticsPlayerController::OnDeploymentUnitClicked);
			DeploymentWidget->OnConfirmPressed.AddDynamic(this, &ALFPTacticsPlayerController::ConfirmDeployment);
			RefreshDeploymentDeckPreview();
			BattleHUDWidget->ShowDeploymentWidget();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("布置阶段：%d 个出生点，%d 个出战单位，%d 个备战单位"),
		PlayerSpawnTiles.Num(),
		GI ? GI->PartyUnits.Num() : 0,
		GI ? GI->ReserveUnits.Num() : 0);
}

void ALFPTacticsPlayerController::AutoPlacePartyUnits()
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !GI->UnitRegistry) return;

	DeployedUnits.SetNum(DeploymentUnitEntries.Num());

	for (int32 i = 0; i < GI->PartyUnits.Num(); i++)
	{
		const FLFPUnitEntry& Entry = GI->PartyUnits[i];
		if (!Entry.IsValid()) continue;

		TSubclassOf<ALFPTacticsUnit> UnitClass = GI->UnitRegistry->GetUnitClass(Entry.TypeID);
		if (!UnitClass) continue;

		// 确定放置位置
		FVector UnitSpawnLocation = FVector(0, 0, 50.f);
		if (PlayerSpawnTiles.IsValidIndex(i))
		{
			UnitSpawnLocation = PlayerSpawnTiles[i]->GetActorLocation() + FVector(0, 0, 50.f);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ALFPTacticsUnit* Unit = GetWorld()->SpawnActor<ALFPTacticsUnit>(UnitClass, UnitSpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (Unit)
		{
			Unit->Affiliation = EUnitAffiliation::UA_Player;
			Unit->UnitTypeID = Entry.TypeID;
			Unit->UnitTier = Entry.Tier;
			Unit->InitializeFromRegistry(GI->UnitRegistry);
			GI->ApplyOwnedRelicsToUnit(Unit);

			// 放置到对应出生点格子
			if (PlayerSpawnTiles.IsValidIndex(i))
			{
				ALFPHexTile* Tile = PlayerSpawnTiles[i];
				Unit->SetCurrentCoordinates(Tile->GetCoordinates());
				Tile->SetIsOccupied(true);
				Tile->SetUnitOnTile(Unit);
			}

			DeployedUnits[i] = Unit;
			UnitToPartyIndex.Add(Unit, i);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("自动放置 %d 个出战单位"), DeployedUnits.Num());
}

// ============== 选中逻辑 ==============

void ALFPTacticsPlayerController::SelectDeployedUnit(ALFPTacticsUnit* Unit)
{
	if (!Unit) return;

	int32* FoundIndex = UnitToPartyIndex.Find(Unit);
	if (!FoundIndex) return;

	// 设置地图选中，清除图标选中（互斥）
	SelectedMapUnitIndex = *FoundIndex;

	// 清除 Widget 图标高亮
	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->ClearAllSelections();
		}
	}

	// 显示格子选中高亮
	UpdateSelectionHighlightForUnit(Unit);

	UE_LOG(LogTemp, Log, TEXT("选中出战单位 %d"), SelectedMapUnitIndex);
}

void ALFPTacticsPlayerController::OnDeploymentTileClicked(ALFPHexTile* Tile)
{
	if (!Tile) return;

	if (!PlayerSpawnTiles.Contains(Tile))
	{
		ClearDeploymentSelection();
		return;
	}

	ALFPTacticsUnit* UnitOnTile = Tile->GetUnitOnTile();
	if (UnitOnTile && UnitToPartyIndex.Contains(UnitOnTile))
	{
		int32 ClickedPartyIndex = UnitToPartyIndex[UnitOnTile];

		if (SelectedMapUnitIndex >= 0 && SelectedMapUnitIndex != ClickedPartyIndex)
		{
			SwapDeployedUnits(SelectedMapUnitIndex, ClickedPartyIndex);
			ClearDeploymentSelection();
		}
		else
		{
			SelectDeployedUnit(UnitOnTile);
		}
		return;
	}

	if (!Tile->IsOccupied())
	{
		if (SelectedMapUnitIndex >= 0)
		{
			MoveDeployedUnitToTile(SelectedMapUnitIndex, Tile);
			ClearDeploymentSelection();
		}
		else
		{
			ClearDeploymentSelection();
		}
	}
}

// ============== UI 委托回调 ==============

ALFPHexTile* ALFPTacticsPlayerController::FindFirstEmptyPlayerSpawnTile() const
{
	for (ALFPHexTile* Tile : PlayerSpawnTiles)
	{
		if (Tile && !Tile->IsOccupied())
		{
			return Tile;
		}
	}

	return nullptr;
}

int32 ALFPTacticsPlayerController::GetCurrentDeployedUnitCount() const
{
	int32 Count = 0;
	for (const TObjectPtr<ALFPTacticsUnit>& UnitPtr : DeployedUnits)
	{
		if (UnitPtr)
		{
			++Count;
		}
	}

	return Count;
}

ALFPTacticsUnit* ALFPTacticsPlayerController::SpawnDeploymentUnit(const FLFPUnitEntry& Entry, ALFPHexTile* Tile)
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !GI->UnitRegistry || !Entry.IsValid() || !Tile)
	{
		return nullptr;
	}

	TSubclassOf<ALFPTacticsUnit> UnitClass = GI->UnitRegistry->GetUnitClass(Entry.TypeID);
	if (!UnitClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALFPTacticsUnit* Unit = GetWorld()->SpawnActor<ALFPTacticsUnit>(
		UnitClass,
		Tile->GetActorLocation() + FVector(0, 0, 50.f),
		FRotator::ZeroRotator,
		SpawnParams);

	if (!Unit)
	{
		return nullptr;
	}

	Unit->Affiliation = EUnitAffiliation::UA_Player;
	Unit->UnitTypeID = Entry.TypeID;
	Unit->UnitTier = Entry.Tier;
	Unit->InitializeFromRegistry(GI->UnitRegistry);
	GI->ApplyOwnedRelicsToUnit(Unit);
	Unit->SetCurrentCoordinates(Tile->GetCoordinates());

	Tile->SetIsOccupied(true);
	Tile->SetUnitOnTile(Unit);

	return Unit;
}

void ALFPTacticsPlayerController::RemoveDeploymentUnitAt(int32 UnitIndex)
{
	if (!DeployedUnits.IsValidIndex(UnitIndex))
	{
		return;
	}

	ALFPTacticsUnit* Unit = DeployedUnits[UnitIndex];
	if (!Unit)
	{
		return;
	}

	if (SelectedMapUnitIndex == UnitIndex)
	{
		ClearDeploymentSelection();
	}

	UnitToPartyIndex.Remove(Unit);
	if (ALFPHexTile* Tile = Unit->GetCurrentTile())
	{
		Tile->SetIsOccupied(false);
		Tile->SetUnitOnTile(nullptr);
	}

	Unit->Affiliation = EUnitAffiliation::UA_Neutral;
	Unit->Destroy();
	DeployedUnits[UnitIndex] = nullptr;
	RefreshDeploymentUnitIcon(UnitIndex);
	RefreshDeploymentDeckPreview();
}

void ALFPTacticsPlayerController::DeployUnitAtFirstEmptySpawn(int32 UnitIndex)
{
	if (!DeploymentUnitEntries.IsValidIndex(UnitIndex) || !DeployedUnits.IsValidIndex(UnitIndex))
	{
		return;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || GetCurrentDeployedUnitCount() >= GI->MaxPartySize)
	{
		return;
	}

	ALFPHexTile* Tile = FindFirstEmptyPlayerSpawnTile();
	if (!Tile)
	{
		return;
	}

	ALFPTacticsUnit* Unit = SpawnDeploymentUnit(DeploymentUnitEntries[UnitIndex], Tile);
	if (!Unit)
	{
		return;
	}

	DeployedUnits[UnitIndex] = Unit;
	UnitToPartyIndex.Add(Unit, UnitIndex);
	RefreshDeploymentUnitIcon(UnitIndex);
	RefreshDeploymentDeckPreview();
}

void ALFPTacticsPlayerController::RefreshDeploymentUnitIcon(int32 UnitIndex)
{
	if (!BattleHUDWidget)
	{
		return;
	}

	if (ULFPDeploymentWidget* DeploymentWidget = BattleHUDWidget->GetDeploymentWidget())
	{
		const bool bIsDeployed = DeployedUnits.IsValidIndex(UnitIndex) && DeployedUnits[UnitIndex] != nullptr;
		DeploymentWidget->SetUnitDeployed(UnitIndex, bIsDeployed);
	}
}

void ALFPTacticsPlayerController::GetRuntimeDeployedUnits(TArray<ALFPTacticsUnit*>& OutUnits) const
{
	OutUnits.Reset();
	OutUnits.Reserve(DeployedUnits.Num());
	for (const TObjectPtr<ALFPTacticsUnit>& UnitPtr : DeployedUnits)
	{
		if (ALFPTacticsUnit* Unit = UnitPtr.Get())
		{
			OutUnits.Add(Unit);
		}
	}
}

void ALFPTacticsPlayerController::RefreshDeploymentDeckPreview()
{
	if (!BattleHUDWidget || !BattleCardComponent)
	{
		return;
	}

	ULFPDeploymentWidget* DeploymentWidget = BattleHUDWidget->GetDeploymentWidget();
	if (!DeploymentWidget)
	{
		return;
	}

	TArray<ALFPTacticsUnit*> RuntimeDeployedUnits;
	GetRuntimeDeployedUnits(RuntimeDeployedUnits);

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	const TArray<FLFPCardInstance> PreviewCards =
		BattleCardComponent->BuildBattleDeckPreview(GI, RuntimeDeployedUnits);
	DeploymentWidget->RefreshDeckPreview(PreviewCards, this);
}

void ALFPTacticsPlayerController::RebuildDeploymentRoster(ULFPGameInstance* GI) const
{
	if (!GI)
	{
		return;
	}

	GI->PartyUnits.Empty();
	GI->ReserveUnits.Empty();

	for (int32 i = 0; i < DeploymentUnitEntries.Num(); ++i)
	{
		if (!DeploymentUnitEntries[i].IsValid())
		{
			continue;
		}

		if (DeployedUnits.IsValidIndex(i) && DeployedUnits[i])
		{
			GI->PartyUnits.Add(DeploymentUnitEntries[i]);
		}
		else
		{
			GI->ReserveUnits.Add(DeploymentUnitEntries[i]);
		}
	}
}

void ALFPTacticsPlayerController::MoveDeployedUnitToTile(int32 PartyIndex, ALFPHexTile* TargetTile)
{
	if (!DeployedUnits.IsValidIndex(PartyIndex)) return;
	ALFPTacticsUnit* Unit = DeployedUnits[PartyIndex];
	if (!Unit || !TargetTile) return;
	if (TargetTile->IsOccupied()) return;

	// 清除旧格子占用
	ALFPHexTile* OldTile = Unit->GetCurrentTile();
	if (OldTile)
	{
		OldTile->SetIsOccupied(false);
		OldTile->SetUnitOnTile(nullptr);
	}

	// 设置新格子
	TargetTile->SetIsOccupied(true);
	TargetTile->SetUnitOnTile(Unit);

	Unit->SetActorLocation(TargetTile->GetActorLocation() + FVector(0, 0, 50.f));
	Unit->SetCurrentCoordinates(TargetTile->GetCoordinates());

	UE_LOG(LogTemp, Log, TEXT("移动单位 %d 到 (%d, %d)"),
		PartyIndex, TargetTile->GetCoordinates().Q, TargetTile->GetCoordinates().R);
}

void ALFPTacticsPlayerController::SwapDeployedUnits(int32 PartyIndexA, int32 PartyIndexB)
{
	if (PartyIndexA == PartyIndexB) return;
	if (!DeployedUnits.IsValidIndex(PartyIndexA) || !DeployedUnits.IsValidIndex(PartyIndexB)) return;

	ALFPTacticsUnit* UnitA = DeployedUnits[PartyIndexA];
	ALFPTacticsUnit* UnitB = DeployedUnits[PartyIndexB];
	if (!UnitA || !UnitB) return;

	ALFPHexTile* TileA = UnitA->GetCurrentTile();
	ALFPHexTile* TileB = UnitB->GetCurrentTile();

	// 交换地图位置（bUpdateOccupancy=false，避免两次 SetCurrentCoordinates 默认清除逻辑互相覆盖）
	if (TileA)
	{
		UnitB->SetCurrentCoordinates(TileA->GetCoordinates(), false);
		TileA->SetIsOccupied(true);
		TileA->SetUnitOnTile(UnitB);
	}
	if (TileB)
	{
		UnitA->SetCurrentCoordinates(TileB->GetCoordinates(), false);
		TileB->SetIsOccupied(true);
		TileB->SetUnitOnTile(UnitA);
	}

	UE_LOG(LogTemp, Log, TEXT("交换单位 %d 和 %d 的地图位置"), PartyIndexA, PartyIndexB);
}

void ALFPTacticsPlayerController::OnDeploymentUnitClicked(int32 UnitIndex)
{
	ClearDeploymentSelection();

	if (!DeploymentUnitEntries.IsValidIndex(UnitIndex))
	{
		return;
	}

	if (DeployedUnits.IsValidIndex(UnitIndex) && DeployedUnits[UnitIndex])
	{
		RemoveDeploymentUnitAt(UnitIndex);
	}
	else
	{
		DeployUnitAtFirstEmptySpawn(UnitIndex);
	}
}

void ALFPTacticsPlayerController::ClearDeploymentSelection()
{
	SelectedMapUnitIndex = -1;

	ClearSelectionHighlight();

	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->ClearAllSelections();
		}
	}
}

void ALFPTacticsPlayerController::ConfirmDeployment()
{
	if (!bIsInDeployment) return;

	// 清除高亮
	if (GridManager)
	{
		GridManager->ClearAllHighlights();
	}
	ClearSelectionHighlight();
	SelectedMapUnitIndex = -1;
	PlayerSpawnTiles.Empty();

	// 隐藏布置 UI
	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* DeploymentWidget = BattleHUDWidget->GetDeploymentWidget())
		{
			DeploymentWidget->OnUnitClicked.RemoveDynamic(this, &ALFPTacticsPlayerController::OnDeploymentUnitClicked);
			DeploymentWidget->OnConfirmPressed.RemoveDynamic(this, &ALFPTacticsPlayerController::ConfirmDeployment);
		}
		BattleHUDWidget->HideDeploymentWidget();
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	RebuildDeploymentRoster(GI);

	if (BattleCardComponent)
	{
		TArray<ALFPTacticsUnit*> RuntimeDeployedUnits;
		GetRuntimeDeployedUnits(RuntimeDeployedUnits);

		// 部署确认后出战单位已固定，此时组装“玩家牌库 + 单位携带卡 + 默认普攻卡”。
		BattleCardComponent->InitializeBattleDeck(GI, RuntimeDeployedUnits);
	}

	bIsInDeployment = false;

	// 通知 TurnManager 结束布置阶段
	ALFPTurnManager* TurnManager = GetTurnManager();
	if (TurnManager)
	{
		TurnManager->EndDeploymentPhase();
	}

	UE_LOG(LogTemp, Log, TEXT("布置确认，进入战斗"));
}
