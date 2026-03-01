// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/UI/Fighting/LFPSkillSelectionWidget.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "Kismet/GameplayStatics.h"
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
    }
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
            ControlledPawn->SetActorLocation(FMath::VInterpTo(
                ControlledPawn->GetActorLocation(),
                TargetLocation,
                DeltaTime,
                5.0f // 插值速度
            ));
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
    if (SelectedUnit)
    {
        bIsAttacking = true;
        CurrentControlState = EPlayControlState::SkillReleaseState;
        ShowUnitRange(EUnitRange::UR_Attack);
    }
}

void ALFPTacticsPlayerController::OnConfirmAction(const FInputActionValue& Value)
{
    bIsDragging = false;
    if (DragTime > DragThresholdTime)
    {
        DragTime = 0.f;
        return;
    }
    DragTime = 0.f;

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
    bIsDragging = false;
    if (DragTime > DragThresholdTime)
    {
        DragTime = 0.f;
        return;
    }
    DragTime = 0.f;

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

    GridManager->ResetGridSprite();
    /*for (ALFPHexTile* Tile : CacheRangeTiles)
    {
        Tile->SetRangeSprite(EUnitRange::UR_Default);
    }*/
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
			// 高亮显示这些格子
            GridManager->UpdateGridSpriteWithTiles(CurrentControlState, MovementRangeTiles);
		}
        break;
	}
    case EUnitRange::UR_Attack:
		// 获取可攻击范围
        CacheRangeTiles = SelectedUnit->GetAttackRangeTiles();
		// 高亮显示这些格子
		for (ALFPHexTile* Tile : CacheRangeTiles)
		{
			Tile->SetRangeSprite(EUnitRange::UR_Attack);
		}
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

        // 高亮显示路径（设置不同颜色）
        for (ALFPHexTile* Tile : CurrentPath)
        {
            Tile->SetPathHighlight(true);
        }
    }
}

void ALFPTacticsPlayerController::HidePathToDefault()
{
    for (ALFPHexTile* Tile : CurrentPath)
    {
        Tile->SetMovementHighlight(false);
    }
    CurrentPath.Empty();
}

void ALFPTacticsPlayerController::HidePathToRange()
{
    for (ALFPHexTile* Tile : CurrentPath)
    {
        Tile->SetMovementHighlight(true);
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

    // 计算移动代价
    const int32 MoveCost = 1;

    // 执行移动
    Unit->MoveToTile(TargetTile);

    // 通知回合管理器单位完成行动
    /*if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->OnUnitFinishedAction(Unit);
    }*/
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
        GridManager->UpdateGridSpriteWithCoords(CurrentControlState, TargetTilesCoord);
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
    if (!EnemyUnit || !EnemyUnit->HasActionPlan()) return;

    PreviewedEnemy = EnemyUnit;
    const FEnemyActionPlan& Plan = EnemyUnit->CurrentActionPlan;

    // 高亮技能效果范围格子
    PreviewEffectTiles = Plan.EffectAreaTiles;
    for (ALFPHexTile* Tile : PreviewEffectTiles)
    {
        if (Tile)
        {
            Tile->SetRangeSprite(EUnitRange::UR_SkillEffect);
        }
    }
}

void ALFPTacticsPlayerController::HideEnemyPlanPreview()
{
    // 恢复预览格子为默认状态
    for (ALFPHexTile* Tile : PreviewEffectTiles)
    {
        if (Tile)
        {
            Tile->SetRangeSprite(EUnitRange::UR_Default);
        }
    }
    PreviewEffectTiles.Empty();
    PreviewedEnemy = nullptr;
}

void ALFPTacticsPlayerController::OnPhaseChanged(EBattlePhase NewPhase)
{
    CachedBattlePhase = NewPhase;

    // 阶段切换时清理预览状态
    HideEnemyPlanPreview();

    switch (NewPhase)
    {
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
