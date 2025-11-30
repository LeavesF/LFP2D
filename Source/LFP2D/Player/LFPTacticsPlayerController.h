// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "InputActionValue.h"
//#include "LFP2D/HexGrid/LFPHexGridManager.h"
//#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPTacticsPlayerController.generated.h"

class ALFPTacticsUnit;
class ALFPHexGridManager;

class UInputMappingContext;
class UInputAction;

class ULFPSkillBase;

class ALFPTurnManager;

class ULFPTurnSpeedListWidget;
class ULFPSkillSelectionWidget;
/**
 * 
 */
UCLASS()
class LFP2D_API ALFPTacticsPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    ALFPTacticsPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

public:
    // Enhanced Input 绑定函数
    void OnSelectStarted(const FInputActionValue& Value);
    void OnSelectCompleted(const FInputActionValue& Value);
    void OnAttackStarted(const FInputActionValue& Value);
    void OnConfirmAction(const FInputActionValue& Value);
    void OnCancelAction(const FInputActionValue& Value);
    //void OnRotateCamera(const FInputActionValue& Value);
    void OnToggleDebug(const FInputActionValue& Value);
    void OnSkipTurnAction(const FInputActionValue& Value);

    void OnCameraPan(const FInputActionValue& Value);
    void OnCameraDragStarted(const FInputActionValue& Value);
    void OnCameraDragTriggered(const FInputActionValue& Value);
    void OnCameraDragCompleted(const FInputActionValue& Value);
    void OnCameraZoom(const FInputActionValue& Value);

    // 游戏流程函数
    void SelectUnit(ALFPTacticsUnit* Unit);
    void SelectTile(ALFPHexTile* Tile);
    void ConfirmMove();
    void ShowUnitRange(EUnitRange UnitRange = EUnitRange::UR_Default);
    void ShowPathToSelectedTile();
    void HidePathToDefault();
    void HidePathToRange();

    // 调试功能
    void ToggleDebugDisplay();

protected:
    // 输入系统资产
    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultInputMapping;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* SelectAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* AttackAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ConfirmAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CancelAction;

    //UPROPERTY(EditAnywhere, Category = "Input")
    //UInputAction* RotateCameraAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* DebugToggleAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SkipTurnAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CameraPanAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CameraDragAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CameraZoomAction;


    // 引用网格管理器
    UPROPERTY()
    ALFPHexGridManager* GridManager;

    // 当前选中的单位
    UPROPERTY()
    ALFPTacticsUnit* SelectedUnit;

    UPROPERTY()
    ALFPTacticsUnit* TargetUnit;

    // 当前选中的格子（用于移动目标）
    UPROPERTY()
    ALFPHexTile* SelectedTile;

    UPROPERTY()
    ALFPHexTile* LastHoveredTile;

    // 当前显示的可移动范围
    TArray<ALFPHexTile*> MovementRangeTiles;

    TArray<ALFPHexTile*> CacheRangeTiles;
    // 当前显示的路径
    TArray<ALFPHexTile*> CurrentPath;

    // 状态标志
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    bool bIsSelecting;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    bool bIsReleaseSkill = false;

    FVector2D SelectionStart;

    // 相机控制
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    float CameraRotationPitchAngle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    float CameraRotationYawAngle;
    FVector CameraOffset;

    // 拖拽状态
    bool bIsDragging;
    FVector2D DragStartPosition;

	UPROPERTY(EditAnywhere, Category = "Input")
	float DragTime = 0.f;

	UPROPERTY(EditAnywhere, Category = "Input")
	float DragThresholdTime = 0.5f;

    // 调试标志
    bool bDebugEnabled;

    // 相机移动速度
    UPROPERTY(EditAnywhere, Category = "Camera")
    float CameraPanSpeed = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float CameraDragSpeed = 10.0f;

    // 相机缩放
    UPROPERTY(EditAnywhere, Category = "Camera")
    float CameraZoomSpeed = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MinZoomDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MaxZoomDistance = 2000.0f;

    float CurrentZoom = 1000.0f;

public:
    ALFPTurnManager* GetTurnManager() const;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void HandleSkillSelection();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void HideSkillSelection();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void HandleSkillTargetSelection(ULFPSkillBase* Skill);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void HandleTargetSelected(ALFPHexTile* TargetTile);

    // 回合事件
    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnTurnStarted(ALFPTacticsUnit* Unit);

    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnTurnEnded(ALFPTacticsUnit* Unit);

    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnRoundStarted(int32 RoundNumber);

    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnRoundEnded(int32 RoundNumber);

    // 单位行动
    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    void MoveUnit(ALFPTacticsUnit* Unit, ALFPHexTile* TargetTile);

    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    bool AttackTarget(ALFPTacticsUnit* Attacker, ALFPTacticsUnit* Target);

    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    void SkipTurn(ALFPTacticsUnit* Unit);

// UI相关
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> TurnSpeedWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TObjectPtr<ULFPTurnSpeedListWidget> TurnSpeedListWidget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UUserWidget> SkillSelectionWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")

    TObjectPtr<ULFPSkillSelectionWidget> SkillSelectionWidget;

protected:
    TObjectPtr<ULFPSkillBase> CurrentSelectedSkill;

/////////// Skill part  ////////////
public:
    UFUNCTION(BlueprintCallable, Category = "Battle System")
    void ExecuteSkill(ULFPSkillBase* CurrentSkill);
};
