// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/UI/Fighting/LFPBattleHUDWidget.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "LFP2D/UI/Fighting/LFPSkillSelectionWidget.h"
#include "LFP2D/UI/Fighting/LFPDeploymentWidget.h"
#include "LFP2D/HexGrid/LFPMapEditorComponent.h"
#include "LFP2D/UI/MapEditor/LFPMapEditorWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
//#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

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

void ALFPTacticsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 绑定Enhanced Input组件
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 选择操作
		EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnSelectStarted);
		EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnSelectCompleted);

		// 其他操作
		EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnConfirmAction);
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
			if (CachedBattlePhase == EBattlePhase::BP_EnemyPlanning || CachedBattlePhase == EBattlePhase::BP_ActionPhase)
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

// 选择开始
void ALFPTacticsPlayerController::OnSelectStarted(const FInputActionValue& Value)
{
	if (bWaitingForMove) return;
	if (bIsInDeployment) return;

	bIsSelecting = true;

	// 获取鼠标位置
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		SelectionStart = FVector2D(MouseX, MouseY);
	}
}

// 选择完成
void ALFPTacticsPlayerController::OnSelectCompleted(const FInputActionValue& Value)
{
	if (bWaitingForMove) return;
	if (bIsInDeployment) return;
	if (!bIsSelecting) return;
	bIsSelecting = false;

	// 获取鼠标位置
	float MouseX, MouseY;
	GetMousePosition(MouseX, MouseY);
	FVector2D SelectionEnd = FVector2D(MouseX, MouseY);

	// 简单的点击检测（非拖拽）
	if (FVector2D::Distance(SelectionStart, SelectionEnd) < 10.0f)
	{
		FHitResult HitResult;
		GetHitResultAtScreenPosition(SelectionEnd, ECC_Visibility, false, HitResult);

		if (HitResult.bBlockingHit)
		{
			// 尝试选中单位
			ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
			if (Unit)
			{
				SelectUnit(Unit);
				return;
			}

			// 尝试选中格子
			ALFPHexTile* Tile = Cast<ALFPHexTile>(HitResult.GetActor());
			if (Tile)
			{
				SelectTile(Tile);
				return;
			}
		}
	}
}

void ALFPTacticsPlayerController::OnConfirmAction(const FInputActionValue& Value)
{
	if (bWaitingForMove) return;
	bIsDragging = false;
	if (DragTime > DragThresholdTime)
	{
		DragTime = 0.f;
		return;
	}
	DragTime = 0.f;

	// 布置阶段优先处理
	if (bIsInDeployment)
	{
		FHitResult HitResult;
		GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

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
		ALFPHexTile* ClickedTile = Cast<ALFPHexTile>(HitResult.GetActor());
		if (ClickedTile)
		{
			OnDeploymentTileClicked(ClickedTile);
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

	if (SelectedUnit && SelectedTile)
	{
		if (bIsReleaseSkill && CurrentSelectedSkill)
		{
			ExecuteSkill(CurrentSelectedSkill);
		}
		else
		{
			ConfirmMove();
		}
	}
	else if (SelectedUnit)
	{
		// 如果没有选中目标格子，显示可移动范围
		//ShowMovementRange(true);
	}
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

	// 布置阶段：清除选中
	if (bIsInDeployment)
	{
		ClearDeploymentSelection();
		return;
	}

	// 技能释放状态：取消技能选择，回到移动预览状态
	if (bIsReleaseSkill)
	{
		bIsReleaseSkill = false;
		CurrentControlState = EPlayControlState::MoveState;
		CurrentSelectedSkill = nullptr;
		if (GridManager)
		{
			GridManager->ClearRangeHighlight(EUnitRange::UR_SkillRelease);
			GridManager->ClearRangeHighlight(EUnitRange::UR_SkillEffect);
		}
		ShowUnitRange(EUnitRange::UR_Move);
		return;
	}

	// 预览移动状态：回到原始位置
	if (SelectedUnit && SelectedUnit->bHasPreviewMoved)
	{
		SelectedUnit->RevertToOriginalPosition();
		ShowUnitRange(EUnitRange::UR_Move);
		return;
	}

	// 未预览移动：不清除单位选中，仅清除路径预览
	HidePathToRange();
	SelectedTile = nullptr;
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
		ShowUnitRange(EUnitRange::UR_Default);
	}

	// 选择新单位
	SelectedUnit = Unit;
	if (SelectedUnit)
	{
		SelectedUnit->SetSelected(true);
		ShowUnitRange(EUnitRange::UR_Move);
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
	if (SelectedTile->IsOccupied() || !SelectedTile->IsWalkable())
	{
		return;
	}
	// 检查目标格是否在原始移动范围内
	if (!MovementRangeTiles.Contains(SelectedTile))
	{
		return;
	}
	// 约束寻路验证：在原始范围内查找路径（找到不到合法路径则阻止移动）
	if (GridManager)
	{
		ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
		if (UnitTile)
		{
			TArray<ALFPHexTile*> ConstrainedPath = GridManager->FindPath(UnitTile, SelectedTile, &MovementRangeTiles);
			if (ConstrainedPath.Num() == 0)
			{
				return;
			}
		}
	}
	// 预览移动：只清除路径高亮，保留原始移动范围
	HidePathToRange();

	// 将移动范围约束写入单位，MoveToTile 将在此范围内寻路
	SelectedUnit->MovementRangeTiles = MovementRangeTiles;

	// 移动单位（预览移动，不消耗移动力）
	MoveUnit(SelectedUnit, SelectedTile);

	CurrentPath.Empty();
	/*SelectedUnit->SetSelected(false);
	SelectedUnit = nullptr;*/
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
		// 使用回合原始位置计算移动范围（预览移动后仍显示原始范围）
		ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->OriginalTurnCoordinates);
		if (UnitTile)
		{
			CacheRangeTiles = MovementRangeTiles = GridManager->GetTilesInRange(UnitTile, SelectedUnit->GetMovementRange());
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
			CurrentPath = GridManager->FindPath(UnitTile, SelectedTile, &MovementRangeTiles);
		}
		else
		{
			CurrentPath = GridManager->FindPath(UnitTile, SelectedTile);
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

	// 解绑委托
	if (MovingUnit)
	{
		MovingUnit->OnMoveFinished.RemoveDynamic(this, &ALFPTacticsPlayerController::OnUnitMoveComplete);
	}

	// 预览移动完成后重新显示原始移动范围
	if (SelectedUnit && SelectedUnit == MovingUnit)
	{
		ShowUnitRange(EUnitRange::UR_Move);
	}

	MovingUnit = nullptr;
}

void ALFPTacticsPlayerController::SkipTurn(ALFPTacticsUnit* Unit)
{

	if (!Unit || !Unit->CanAct()) return;

	// 提交预览移动（消耗移动力）
	Unit->CommitMovePosition();
	Unit->SetHasActed(true);

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
		// 提交预览移动（消耗移动力）
		SelectedUnit->CommitMovePosition();
		ALFPHexTile* TargetTile = SelectedTile;
		if (CurrentSkill->TargetType == ESkillTargetType::Self)
		{
			TargetTile = SelectedUnit->GetCurrentTile();
		}

		if (SelectedUnit->ExecuteSkill(CurrentSkill, TargetTile))
		{
			bIsReleaseSkill = false;
			SelectedUnit->ConsumeActionPoints(CurrentSkill->ActionPointCost);
			CurrentSelectedSkill = nullptr;
			SelectedTile = nullptr;
			CurrentControlState = EPlayControlState::MoveState;
			ShowUnitRange(EUnitRange::UR_Move);
			SkipTurn(SelectedUnit);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("玩家主动释放失败: 技能[%s]"), *CurrentSkill->SkillName.ToString());
		}
	}
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

	ULFPSkillSelectionWidget* SkillWidget = BattleHUDWidget->GetSkillSelectionWidget();
	if (SkillWidget)
	{
		BattleHUDWidget->ShowSkillSelection();
		SkillWidget->InitializeSkillsInfo(SelectedUnit, this);
	}
}

void ALFPTacticsPlayerController::HideSkillSelection()
{
	if (BattleHUDWidget)
	{
		BattleHUDWidget->HideSkillSelection();
	}
}

void ALFPTacticsPlayerController::HandleSkillTargetSelecting(ULFPSkillBase* Skill)
{
	if (!SelectedUnit || !Skill) return;
	if (Skill->IsPassiveSkill()) return;

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
		OnDeploymentPhaseStarted();
		break;

	case EBattlePhase::BP_EnemyPlanning:
		// 敌人规划阶段：隐藏技能选择UI
		HideSkillSelection();
		break;

	case EBattlePhase::BP_ActionPhase:
		// 行动阶段：恢复正常
		break;

	case EBattlePhase::BP_RoundEnd:
		break;
	}
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
	SelectedPartyIconIdx = -1;
	SelectedReserveIndex = -1;
	HighlightedSelectionTile = nullptr;

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
			DeploymentWidget->OnPartyUnitClicked.AddDynamic(this, &ALFPTacticsPlayerController::OnDeploymentPartyUnitClicked);
			DeploymentWidget->OnReserveUnitClicked.AddDynamic(this, &ALFPTacticsPlayerController::OnDeploymentReserveUnitClicked);
			DeploymentWidget->OnConfirmPressed.AddDynamic(this, &ALFPTacticsPlayerController::ConfirmDeployment);
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

	DeployedUnits.SetNum(GI->PartyUnits.Num());

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
	SelectedPartyIconIdx = -1;
	SelectedReserveIndex = -1;

	// 清除 Widget 图标高亮
	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->ClearAllSelections();
		}
	}

	// 显示格子选中高亮
	if (ALFPHexTile* UnitTile = Unit->GetCurrentTile())
	{
		if (HighlightedSelectionTile && HighlightedSelectionTile != UnitTile)
		{
			HighlightedSelectionTile->ShowSelectionHighlight(false);
		}
		UnitTile->ShowSelectionHighlight(true);
		HighlightedSelectionTile = UnitTile;
	}

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

void ALFPTacticsPlayerController::OnDeploymentPartyUnitClicked(int32 PartyIndex)
{
	if (!DeployedUnits.IsValidIndex(PartyIndex) || !DeployedUnits[PartyIndex]) return;

	// 地图单位选中 → 点击图标 → 覆盖（改为选中图标）
	if (SelectedMapUnitIndex >= 0)
	{
		ClearDeploymentSelection();
	}

	// 备战图标选中 → 点击出战图标 → 替换
	if (SelectedReserveIndex >= 0)
	{
		ReplacePartyWithReserve(PartyIndex, SelectedReserveIndex);
		return;
	}

	// 再次点击同一图标 → 取消选中
	if (SelectedPartyIconIdx == PartyIndex)
	{
		ClearDeploymentSelection();
		return;
	}

	// 点击不同出战图标 → 同席交换（仅图标，场上单位不动）
	if (SelectedPartyIconIdx >= 0)
	{
		SwapPartyUnitIcons(SelectedPartyIconIdx, PartyIndex);
		ClearDeploymentSelection();
		return;
	}

	// 选中出战图标
	SelectedPartyIconIdx = PartyIndex;
	SelectedMapUnitIndex = -1;
	SelectedReserveIndex = -1;

	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->ClearAllSelections();
			Dw->MarkPartyUnitSelected(PartyIndex, true);
		}
	}
}

void ALFPTacticsPlayerController::OnDeploymentReserveUnitClicked(int32 ReserveIndex)
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !GI->ReserveUnits.IsValidIndex(ReserveIndex)) return;
	if (!GI->ReserveUnits[ReserveIndex].IsValid()) return;

	// 地图单位选中 → 点击图标 → 覆盖
	if (SelectedMapUnitIndex >= 0)
	{
		ClearDeploymentSelection();
	}

	// 出战图标选中 → 点击备战图标 → 替换
	if (SelectedPartyIconIdx >= 0)
	{
		ReplacePartyWithReserve(SelectedPartyIconIdx, ReserveIndex);
		return;
	}

	// 再次点击同一图标 → 取消选中
	if (SelectedReserveIndex == ReserveIndex)
	{
		ClearDeploymentSelection();
		return;
	}

	// 点击不同备战图标 → 同席交换
	if (SelectedReserveIndex >= 0)
	{
		SwapReserveUnitIcons(SelectedReserveIndex, ReserveIndex);
		ClearDeploymentSelection();
		return;
	}

	// 选中备战图标
	SelectedReserveIndex = ReserveIndex;
	SelectedPartyIconIdx = -1;
	SelectedMapUnitIndex = -1;

	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->ClearAllSelections();
			Dw->MarkReserveUnitSelected(ReserveIndex, true);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("选中备战单位 %d"), ReserveIndex);
}

// ============== 操作实现 ==============

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


void ALFPTacticsPlayerController::SwapPartyUnitIcons(int32 A, int32 B)
{
	if (A == B) return;

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !GI->PartyUnits.IsValidIndex(A) || !GI->PartyUnits.IsValidIndex(B)) return;

	GI->PartyUnits.Swap(A, B);
	DeployedUnits.Swap(A, B);
	if (DeployedUnits[A]) UnitToPartyIndex[DeployedUnits[A]] = A;
	if (DeployedUnits[B]) UnitToPartyIndex[DeployedUnits[B]] = B;

	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->UpdatePartySlot(A, GI->PartyUnits[A]);
			Dw->UpdatePartySlot(B, GI->PartyUnits[B]);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("交换出战图标 %d 和 %d"), A, B);
}

void ALFPTacticsPlayerController::SwapReserveUnitIcons(int32 A, int32 B)
{
	if (A == B) return;

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !GI->ReserveUnits.IsValidIndex(A) || !GI->ReserveUnits.IsValidIndex(B)) return;

	GI->ReserveUnits.Swap(A, B);

	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->UpdateReserveSlot(A, GI->ReserveUnits[A]);
			Dw->UpdateReserveSlot(B, GI->ReserveUnits[B]);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("交换备战图标 %d 和 %d"), A, B);
}

void ALFPTacticsPlayerController::ReplacePartyWithReserve(int32 PartyIndex, int32 ReserveIndex)
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !GI->UnitRegistry) return;
	if (!GI->PartyUnits.IsValidIndex(PartyIndex)) return;
	if (!GI->ReserveUnits.IsValidIndex(ReserveIndex)) return;

	ALFPTacticsUnit* OldUnit = DeployedUnits.IsValidIndex(PartyIndex) ? DeployedUnits[PartyIndex] : nullptr;
	if (!OldUnit) return;

	// 获取旧单位所在的格子和位置
	ALFPHexTile* Tile = OldUnit->GetCurrentTile();
	FVector UnitSpawnLocation = OldUnit->GetActorLocation();

	// 清除旧单位
	UnitToPartyIndex.Remove(OldUnit);
	if (Tile)
	{
		Tile->SetIsOccupied(false);
		Tile->SetUnitOnTile(nullptr);
	}
	OldUnit->Destroy();

	// 生成新单位（从备战数据）
	const FLFPUnitEntry& ReserveEntry = GI->ReserveUnits[ReserveIndex];
	TSubclassOf<ALFPTacticsUnit> UnitClass = GI->UnitRegistry->GetUnitClass(ReserveEntry.TypeID);
	if (!UnitClass) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALFPTacticsUnit* NewUnit = GetWorld()->SpawnActor<ALFPTacticsUnit>(UnitClass, UnitSpawnLocation, FRotator::ZeroRotator, SpawnParams);

	if (NewUnit)
	{
		NewUnit->Affiliation = EUnitAffiliation::UA_Player;
		NewUnit->UnitTypeID = ReserveEntry.TypeID;
		NewUnit->UnitTier = ReserveEntry.Tier;
		NewUnit->InitializeFromRegistry(GI->UnitRegistry);
		GI->ApplyOwnedRelicsToUnit(NewUnit);

		// 放置到旧单位的位置
		if (Tile)
		{
			NewUnit->SetCurrentCoordinates(Tile->GetCoordinates());
			Tile->SetIsOccupied(true);
			Tile->SetUnitOnTile(NewUnit);
		}

		DeployedUnits[PartyIndex] = NewUnit;
		UnitToPartyIndex.Add(NewUnit, PartyIndex);
	}

	// 交换 GameInstance 数据：PartyUnits[PartyIndex] ↔ ReserveUnits[ReserveIndex]
	FLFPUnitEntry Temp = GI->PartyUnits[PartyIndex];
	GI->PartyUnits[PartyIndex] = GI->ReserveUnits[ReserveIndex];
	GI->ReserveUnits[ReserveIndex] = Temp;

	// 更新 UI
	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* Dw = BattleHUDWidget->GetDeploymentWidget())
		{
			Dw->UpdatePartySlot(PartyIndex, GI->PartyUnits[PartyIndex]);
			// 备战槽位可能变为空（如果出战的槽位原本为空）
			if (GI->ReserveUnits.IsValidIndex(ReserveIndex))
			{
				Dw->UpdateReserveSlot(ReserveIndex, GI->ReserveUnits[ReserveIndex]);
			}
		}
	}

	// 清除所有选中
	ClearDeploymentSelection();

	UE_LOG(LogTemp, Log, TEXT("备战单位 %d 替换出战单位 %d"), ReserveIndex, PartyIndex);
}

void ALFPTacticsPlayerController::ClearDeploymentSelection()
{
	SelectedMapUnitIndex = -1;
	SelectedPartyIconIdx = -1;
	SelectedReserveIndex = -1;

	// 清除格子选中高亮
	if (HighlightedSelectionTile)
	{
		HighlightedSelectionTile->ShowSelectionHighlight(false);
		HighlightedSelectionTile = nullptr;
	}

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
	PlayerSpawnTiles.Empty();

	// 隐藏布置 UI
	if (BattleHUDWidget)
	{
		if (ULFPDeploymentWidget* DeploymentWidget = BattleHUDWidget->GetDeploymentWidget())
		{
			DeploymentWidget->OnPartyUnitClicked.RemoveDynamic(this, &ALFPTacticsPlayerController::OnDeploymentPartyUnitClicked);
			DeploymentWidget->OnReserveUnitClicked.RemoveDynamic(this, &ALFPTacticsPlayerController::OnDeploymentReserveUnitClicked);
			DeploymentWidget->OnConfirmPressed.RemoveDynamic(this, &ALFPTacticsPlayerController::ConfirmDeployment);
		}
		BattleHUDWidget->HideDeploymentWidget();
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
