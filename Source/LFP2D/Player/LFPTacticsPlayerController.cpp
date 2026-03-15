// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/UI/Fighting/LFPSkillSelectionWidget.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
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
    bIsAttacking = false;
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

    // 初始化UI
    if (TurnSpeedWidgetClass)
    {
        TurnSpeedListWidget = CreateWidget<ULFPTurnSpeedListWidget>(this, TurnSpeedWidgetClass);
        if (TurnSpeedListWidget)
        {
            TurnSpeedListWidget->AddToViewport();
            TurnSpeedListWidget->InitializeTurnOrder(); // 初始化单位顺序
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

        // 攻击操作
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnAttackStarted);

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

    // 布置阶段：更新预览单位跟随鼠标
    if (bIsInDeployment && bIsPlacingUnit && PlacingUnitPreview)
    {
        FVector WorldOrigin, WorldDirection;
        DeprojectMousePositionToWorld(WorldOrigin, WorldDirection);
        if (FMath::Abs(WorldDirection.Z) > KINDA_SMALL_NUMBER)
        {
            float T = -WorldOrigin.Z / WorldDirection.Z;
            FVector HitPoint = WorldOrigin + WorldDirection * T;
            PlacingUnitPreview->SetActorLocation(FVector(HitPoint.X, HitPoint.Y, 20.f));
        }
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

                if (HoveredEnemy && HoveredEnemy != PreviewedEnemy)
                {
                    HideEnemyPlanPreview();
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

void ALFPTacticsPlayerController::OnAttackStarted(const FInputActionValue& Value)
{
    if (bWaitingForMove) return;
    if (SelectedUnit)
    {
        bIsAttacking = true;
        CurrentControlState = EPlayControlState::SkillReleaseState;
        ShowUnitRange(EUnitRange::UR_Attack);
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
        if (bIsPlacingUnit)
        {
            // 正在放置单位：检测是否点击了空闲出生点
            if (LastHoveredTile && PlayerSpawnTiles.Contains(LastHoveredTile)
                && !LastHoveredTile->IsOccupied())
            {
                PlaceUnit(LastHoveredTile);
            }
        }
        else
        {
            // 不在放置模式：检测是否点击了已放置的单位（拾起）
            FHitResult HitResult;
            GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
            ALFPTacticsUnit* ClickedUnit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
            if (ClickedUnit && DeployedUnits.Contains(ClickedUnit))
            {
                PickupDeployedUnit(ClickedUnit);
            }
        }
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
        if (bIsAttacking)
        {
            // 获取鼠标位置
            float MouseX, MouseY;
            GetMousePosition(MouseX, MouseY);
            FVector2D SelectionEnd = FVector2D(MouseX, MouseY);

            FHitResult HitResult;
            GetHitResultAtScreenPosition(SelectionEnd, ECC_Visibility, false, HitResult);

            if (HitResult.bBlockingHit)
            {
                // 尝试选中单位
                ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
                if (Unit)
                {
                    AttackTarget(SelectedUnit, Unit);
                    return;
                }
            }
        }
        else if (bIsReleaseSkill && CurrentSelectedSkill)
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

    // 布置阶段：取消放置
    if (bIsInDeployment && bIsPlacingUnit)
    {
        CancelPlacing();
        return;
    }

    bIsAttacking = false;
    bIsReleaseSkill = false;
    CurrentControlState = EPlayControlState::MoveState;
    ShowUnitRange(EUnitRange::UR_Move);
    // 取消当前选择
    if (SelectedTile)
    {
        //HidePathToDefault();
        //SelectedTile = nullptr;
    }
    else if (SelectedUnit)
    {
        // 取消单位的高亮范围
        //ShowMovementRange(false);
        //SelectedUnit->SetSelected(false);
        //SelectedUnit = nullptr;
    }
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
    //HidePathToDefault();
    // 移动前取消高亮
    ShowUnitRange(EUnitRange::UR_Default);

    // 移动单位
    MoveUnit(SelectedUnit, SelectedTile);

    CurrentPath.Empty();

    ShowUnitRange(EUnitRange::UR_Move);
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
        // 获取可移动范围
		ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
		if (UnitTile)
		{
			CacheRangeTiles= MovementRangeTiles = GridManager->GetTilesInRange(UnitTile, SelectedUnit->GetMovementRange());
			// 显示范围高亮（描边 + 填充）
            GridManager->ShowRangeHighlight(MovementRangeTiles, EUnitRange::UR_Move);
		}
        break;
	}
    case EUnitRange::UR_Attack:
		// 获取可攻击范围
        CacheRangeTiles = SelectedUnit->GetAttackRangeTiles();
		// 显示范围高亮（描边 + 填充）
		GridManager->ShowRangeHighlight(CacheRangeTiles, EUnitRange::UR_Attack);
        break;
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
        CurrentPath = GridManager->FindPath(UnitTile, SelectedTile);

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

    // 刷新移动范围显示
    if (SelectedUnit && SelectedUnit == MovingUnit)
    {
        ShowUnitRange(EUnitRange::UR_Default);
    }

    MovingUnit = nullptr;
}

bool ALFPTacticsPlayerController::AttackTarget(ALFPTacticsUnit* Attacker, ALFPTacticsUnit* Target)
{
    if (!Attacker || !Target || !Attacker->CanAct())
    {
        return false;
    }

    // 计算攻击代价
    const int32 AttackCost = 1;

    bIsAttacking = false;
    CurrentControlState = EPlayControlState::MoveState;

    if (!Attacker->HasEnoughActionPoints(AttackCost))
    {
        return false;
    }

    // 执行攻击逻辑
    bool bAttackSucceed = Attacker->AttackTarget(Target);

    if (bAttackSucceed)
    {
		// 消耗行动力
		Attacker->ConsumeActionPoints(AttackCost);
        ShowUnitRange(EUnitRange::UR_Default);
    }
    else
    {
        ShowUnitRange(EUnitRange::UR_Move);
    }

    // 通知回合管理器单位完成行动
    if (!Attacker->HasEnoughActionPoints(1))
    {
		if (bAttackSucceed)
		{
            SkipTurn(Attacker);
		}
    }

    return bAttackSucceed;
}

void ALFPTacticsPlayerController::SkipTurn(ALFPTacticsUnit* Unit)
{
    if (!Unit || !Unit->CanAct()) return;

    // 标记为已行动
    Unit->SetHasActed(true);

    // 可选：消耗1个行动力作为跳过代价
    // Unit->ConsumeMovePoints(1);
    bIsAttacking = false;
    CurrentControlState = EPlayControlState::MoveState;
    // 通知回合管理器单位完成行动
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->OnUnitFinishedAction(Unit);
    }
}

void ALFPTacticsPlayerController::ExecuteSkill(ULFPSkillBase* CurrentSkill)
{
    if (SelectedUnit && SelectedTile && CurrentSkill)
    {
        if (CurrentSkill->CanExecute(SelectedTile))
        {
            bIsReleaseSkill = false;
            CurrentSkill->Execute(SelectedTile);
            SelectedUnit->ConsumeActionPoints(CurrentSkill->ActionPointCost);
            CurrentControlState = EPlayControlState::MoveState;
            ShowUnitRange(EUnitRange::UR_Move);
            SkipTurn(SelectedUnit);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Skill:%s can not release!"), *CurrentSkill->SkillName.ToString());
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

    // 显示技能选择UI
    if (SkillSelectionWidgetClass)
    {
        if (!SkillSelectionWidget)
        {
            SkillSelectionWidget = CreateWidget<ULFPSkillSelectionWidget>(this, SkillSelectionWidgetClass);
            if (SkillSelectionWidget)
            {
                SkillSelectionWidget->Show();
                SkillSelectionWidget->InitializeSkillsInfo(SelectedUnit, this);
                SkillSelectionWidget->AddToViewport();

                //// 进入技能选择模式
                //CurrentSelectionMode = ESelectionMode::SkillSelection;
            }
        }
        else
        {
            SkillSelectionWidget->Show();
            SkillSelectionWidget->InitializeSkillsInfo(SelectedUnit, this);

            //// 进入技能选择模式
            //CurrentSelectionMode = ESelectionMode::SkillSelection;
        }

    }
}

void ALFPTacticsPlayerController::HideSkillSelection()
{
    if (SkillSelectionWidget)
    {
        SkillSelectionWidget->Hide();
    }
}

void ALFPTacticsPlayerController::HandleSkillTargetSelecting(ULFPSkillBase* Skill)
{
    if (!SelectedUnit || !Skill) return;

    // 获取技能范围内的目标格子
    Skill->UpdateSkillRange();
    TArray<FLFPHexCoordinates> TargetTilesCoord = Skill->GetReleaseRange();
    if (GridManager)
    {
        bIsReleaseSkill = true;
        CurrentControlState = EPlayControlState::SkillReleaseState;
        GridManager->ShowRangeHighlightByCoords(TargetTilesCoord, EUnitRange::UR_SkillEffect);
    }
    //// 高亮目标格子
    //if (GridManager)
    //{
    //    GridManager->HighlightTiles(TargetTiles, FLinearColor::Red);
    //}

    // 进入目标选择模式
    //CurrentSelectionMode = ESelectionMode::TargetSelection;
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

    // 高亮技能效果范围格子
    PreviewEffectTiles = Plan.EffectAreaTiles;
    GridManager->ShowRangeHighlight(PreviewEffectTiles, EUnitRange::UR_SkillEffect);
}

void ALFPTacticsPlayerController::HideEnemyPlanPreview()
{
    // 清除预览格子高亮
    if (GridManager)
    {
        GridManager->ClearAllHighlights();
    }
    PreviewEffectTiles.Empty();
    PreviewedEnemy = nullptr;
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
    bIsPlacingUnit = false;
    PlacingPartyIndex = -1;
    PlacingUnitPreview = nullptr;

    // 获取玩家出生点并高亮
    if (GridManager)
    {
        PlayerSpawnTiles = GridManager->GetSpawnPoints(ELFPSpawnFaction::SF_Player);
        // 高亮显示出生点
        GridManager->ShowRangeHighlight(PlayerSpawnTiles, EUnitRange::UR_Move);
    }

    // 获取队伍数据
    ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
    if (!GI) return;

    // 初始化已布置单位数组（与 PartyUnits 等长，初始全为 nullptr）
    DeployedUnits.Empty();
    DeployedUnits.SetNum(GI->PartyUnits.Num());

    // 创建布置 UI
    if (DeploymentWidgetClass && !DeploymentWidget)
    {
        DeploymentWidget = CreateWidget<ULFPDeploymentWidget>(this, DeploymentWidgetClass);
    }

    if (DeploymentWidget)
    {
        DeploymentWidget->Setup(GI->PartyUnits, GI->UnitRegistry);
        DeploymentWidget->OnUnitSelected.AddDynamic(this, &ALFPTacticsPlayerController::StartPlacingUnit);
        DeploymentWidget->OnConfirmPressed.AddDynamic(this, &ALFPTacticsPlayerController::ConfirmDeployment);
        DeploymentWidget->AddToViewport();
    }

    UE_LOG(LogTemp, Log, TEXT("布置阶段：%d 个出生点，%d 个队伍单位"),
        PlayerSpawnTiles.Num(), GI->PartyUnits.Num());
}

void ALFPTacticsPlayerController::StartPlacingUnit(int32 PartyIndex)
{
    // 如果正在放置其他单位，先取消
    if (bIsPlacingUnit)
    {
        CancelPlacing();
    }

    ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
    if (!GI || !GI->UnitRegistry) return;
    if (!GI->PartyUnits.IsValidIndex(PartyIndex)) return;

    // 如果该单位已经放置，先拾起
    if (DeployedUnits.IsValidIndex(PartyIndex) && DeployedUnits[PartyIndex])
    {
        PickupDeployedUnit(DeployedUnits[PartyIndex]);
    }

    const FLFPUnitEntry& Entry = GI->PartyUnits[PartyIndex];
    TSubclassOf<ALFPTacticsUnit> UnitClass = GI->UnitRegistry->GetUnitClass(Entry.TypeID);
    if (!UnitClass) return;

    // 在鼠标位置生成预览单位
    FVector UnitSpawnLoc = FVector(0, 0, 50.f);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    PlacingUnitPreview = GetWorld()->SpawnActor<ALFPTacticsUnit>(UnitClass, UnitSpawnLoc, FRotator::ZeroRotator, SpawnParams);

    if (PlacingUnitPreview)
    {
        PlacingUnitPreview->Affiliation = EUnitAffiliation::UA_Player;
        PlacingUnitPreview->UnitTypeID = Entry.TypeID;
        PlacingUnitPreview->UnitTier = Entry.Tier;
    }

    PlacingPartyIndex = PartyIndex;
    bIsPlacingUnit = true;

    if (DeploymentWidget)
    {
        DeploymentWidget->MarkUnitSelecting(PartyIndex);
    }

    UE_LOG(LogTemp, Log, TEXT("开始放置单位 %s（索引 %d）"), *Entry.TypeID.ToString(), PartyIndex);
}

void ALFPTacticsPlayerController::PlaceUnit(ALFPHexTile* Tile)
{
    if (!Tile || !PlacingUnitPreview || PlacingPartyIndex < 0) return;

    // 放置到格子上
    FVector TileLocation = Tile->GetActorLocation() + FVector(0, 0, 50.f);
    PlacingUnitPreview->SetActorLocation(TileLocation);
    PlacingUnitPreview->SetCurrentCoordinates(Tile->GetCoordinates());

    // 更新格子占用
    Tile->SetIsOccupied(true);
    Tile->SetUnitOnTile(PlacingUnitPreview);

    // 记录到已布置列表
    if (DeployedUnits.IsValidIndex(PlacingPartyIndex))
    {
        DeployedUnits[PlacingPartyIndex] = PlacingUnitPreview;
    }

    // 更新 UI
    if (DeploymentWidget)
    {
        DeploymentWidget->MarkUnitPlaced(PlacingPartyIndex, true);
    }

    UE_LOG(LogTemp, Log, TEXT("放置单位到格子 (%d, %d)"), Tile->GetCoordinates().Q, Tile->GetCoordinates().R);

    // 重置放置状态
    PlacingUnitPreview = nullptr;
    PlacingPartyIndex = -1;
    bIsPlacingUnit = false;
}

void ALFPTacticsPlayerController::CancelPlacing()
{
    if (PlacingUnitPreview)
    {
        PlacingUnitPreview->Destroy();
        PlacingUnitPreview = nullptr;
    }

    PlacingPartyIndex = -1;
    bIsPlacingUnit = false;
}

void ALFPTacticsPlayerController::PickupDeployedUnit(ALFPTacticsUnit* Unit)
{
    if (!Unit) return;

    // 找到对应的 PartyIndex
    int32 FoundIndex = DeployedUnits.Find(Unit);
    if (FoundIndex == INDEX_NONE) return;

    // 清除格子占用
    ALFPHexTile* OldTile = Unit->GetCurrentTile();
    if (OldTile)
    {
        OldTile->SetIsOccupied(false);
        OldTile->SetUnitOnTile(nullptr);
    }

    // 设置为跟随鼠标的预览
    PlacingUnitPreview = Unit;
    PlacingPartyIndex = FoundIndex;
    bIsPlacingUnit = true;

    // 清除已布置记录
    DeployedUnits[FoundIndex] = nullptr;

    // 更新 UI
    if (DeploymentWidget)
    {
        DeploymentWidget->MarkUnitPlaced(FoundIndex, false);
    }

    UE_LOG(LogTemp, Log, TEXT("拾起已放置的单位（索引 %d）"), FoundIndex);
}

void ALFPTacticsPlayerController::ConfirmDeployment()
{
    if (!bIsInDeployment) return;

    // 取消高亮
    // 清除出生点高亮
    GridManager->ClearAllHighlights();
    PlayerSpawnTiles.Empty();

    // 移除布置 UI
    if (DeploymentWidget)
    {
        DeploymentWidget->OnUnitSelected.RemoveDynamic(this, &ALFPTacticsPlayerController::StartPlacingUnit);
        DeploymentWidget->OnConfirmPressed.RemoveDynamic(this, &ALFPTacticsPlayerController::ConfirmDeployment);
        DeploymentWidget->RemoveFromParent();
    }

    bIsInDeployment = false;
    bIsPlacingUnit = false;

    // 通知 TurnManager 结束布置阶段
    ALFPTurnManager* TurnManager = GetTurnManager();
    if (TurnManager)
    {
        TurnManager->EndDeploymentPhase();
    }

    UE_LOG(LogTemp, Log, TEXT("布置确认，进入战斗"));
}
