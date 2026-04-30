// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "InputActionValue.h"
#include "LFPTacticsPlayerController.generated.h"

class ALFPTacticsUnit;
class ALFPHexGridManager;

class UInputMappingContext;
class UInputAction;

class ULFPSkillBase;

class ALFPTurnManager;

class ULFPBattleHUDWidget;
class ULFPMapEditorComponent;
class ULFPMapEditorWidget;
class ULFPUnitRegistryDataAsset;
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
    // Enhanced Input绑定函数
    void OnSelectStarted(const FInputActionValue& Value);
    void OnSelectCompleted(const FInputActionValue& Value);
    void OnConfirmAction(const FInputActionValue& Value);
    void OnCancelAction(const FInputActionValue& Value);
    //void OnRotateCamera(const FInputActionValue& Value);
    void OnToggleDebug(const FInputActionValue& Value);
    void OnSkipTurnAction(const FInputActionValue& Value);
    void OnToggleEditorAction(const FInputActionValue& Value);

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

	// 地图编辑器输入
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ToggleEditorAction;


    // 网格管理器引用
    UPROPERTY()
    ALFPHexGridManager* GridManager;

	// 地图编辑器组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Editor")
	TObjectPtr<ULFPMapEditorComponent> MapEditorComponent;

    // 当前选中的单位
    UPROPERTY()
    ALFPTacticsUnit* SelectedUnit;

    UPROPERTY()
    ALFPTacticsUnit* TargetUnit;

    // 当前选中的格子（即移动目标）
    UPROPERTY()
    ALFPHexTile* SelectedTile;

    UPROPERTY()
    ALFPHexTile* LastHoveredTile;

    // 当前显示的可移动范围（缓存的原始位置范围，用于预览移动时判断合法目标）
    TArray<ALFPHexTile*> MovementRangeTiles;

    TArray<ALFPHexTile*> CacheRangeTiles;
    // 当前显示的路径
    TArray<ALFPHexTile*> CurrentPath;

    // 状态标志
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    bool bIsSelecting;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    bool bIsReleaseSkill = false;

    FVector2D SelectionStart;

    // 相机参数
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

    // 场景切换后首帧跳过相机平滑插值
    bool bSnapCameraNextFrame = true;

    // 等待单位移动动画完成（期间锁定游戏输入）
    bool bWaitingForMove = false;

    // 当前正在移动的单位（用于解绑委托）
    UPROPERTY()
    TObjectPtr<ALFPTacticsUnit> MovingUnit;

    // 单位移动完成回调
    UFUNCTION()
    void OnUnitMoveComplete();

public:
    ALFPTurnManager* GetTurnManager() const;

    // 获取战斗 HUD（供 GameMode 等外部访问）
    UFUNCTION(BlueprintPure, Category = "UI")
    ULFPBattleHUDWidget* GetBattleHUD() const { return BattleHUDWidget; }

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void HideSkillSelection();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void HandleSkillSelection();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void HandleSkillTargetSelecting(ULFPSkillBase* Skill);

    /*UFUNCTION(BlueprintCallable, Category = "Skill")
    void HandleSkillTargetSelected(ALFPHexTile* TargetTile);*/

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
    void SkipTurn(ALFPTacticsUnit* Unit);


	// UI相关
	// 地图编辑器 UI
protected:
	// 战斗 HUD
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<ULFPBattleHUDWidget> BattleHUDClass;

	UPROPERTY()
	TObjectPtr<ULFPBattleHUDWidget> BattleHUDWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<ULFPMapEditorWidget> MapEditorWidgetClass;

	UPROPERTY()
	TObjectPtr<ULFPMapEditorWidget> MapEditorWidget;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TObjectPtr<ULFPSkillBase> CurrentSelectedSkill;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    EPlayControlState LastControlState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    EPlayControlState CurrentControlState;

/////////// Skill part  ////////////
public:
    UFUNCTION(BlueprintCallable, Category = "Battle System")
    void ExecuteSkill(ULFPSkillBase* CurrentSkill);

    // ==== 敌人计划预览 ====
public:
    // 显示敌人的技能效果预览（鼠标悬停时）
    UFUNCTION(BlueprintCallable, Category = "Enemy Preview")
    void ShowEnemyPlanPreview(ALFPTacticsUnit* EnemyUnit);

    // 隐藏敌人的技能效果预览
    UFUNCTION(BlueprintCallable, Category = "Enemy Preview")
    void HideEnemyPlanPreview();

    // 阶段变化响应
    UFUNCTION()
    void OnPhaseChanged(EBattlePhase NewPhase);

protected:
    // 当前预览的敌人
    UPROPERTY()
    ALFPTacticsUnit* PreviewedEnemy = nullptr;

    TArray<ALFPHexTile*> PreviewReleaseTiles;

    // 预览高亮的格子缓存
    TArray<ALFPHexTile*> PreviewEffectTiles;

    // 当前战斗阶段（本地缓存）
    EBattlePhase CachedBattlePhase = EBattlePhase::BP_RoundEnd;

    // ==== 布置阶段 ====
public:
    // 布置阶段开始时调用（自动部署出战队伍、初始化 UI）
    void OnDeploymentPhaseStarted();

    // 自动将所有出战单位放置到出生点
    void AutoPlacePartyUnits();

    // UI 委托：点击出战单位图标
    UFUNCTION()
    void OnDeploymentPartyUnitClicked(int32 PartyIndex);

    // UI 委托：点击备战单位图标
    UFUNCTION()
    void OnDeploymentReserveUnitClicked(int32 ReserveIndex);

    // 确认布置完毕
    UFUNCTION()
    void ConfirmDeployment();

protected:
    // 选中地图上的已部署单位（确认键点击单位）
    void SelectDeployedUnit(ALFPTacticsUnit* Unit);

    // 点击部署格子处理（移动/交换）
    void OnDeploymentTileClicked(ALFPHexTile* Tile);

    // 清除布置阶段选中状态
    void ClearDeploymentSelection();

    // 移动已部署单位到空格子
    void MoveDeployedUnitToTile(int32 PartyIndex, ALFPHexTile* TargetTile);

    // 交换两个已部署单位的位置（地图上互换）
    void SwapDeployedUnits(int32 PartyIndexA, int32 PartyIndexB);

    // 交换两个出战图标（仅 UI，不影响地图位置）
    void SwapPartyUnitIcons(int32 PartyIndexA, int32 PartyIndexB);

    // 交换两个备战图标
    void SwapReserveUnitIcons(int32 PartyIndexA, int32 PartyIndexB);

    // 用备战单位替换出战单位
    void ReplacePartyWithReserve(int32 PartyIndex, int32 ReserveIndex);

    // 布置阶段状态
    bool bIsInDeployment = false;

    // 当前选中的地图单位（确认键点击场上的单位）
    int32 SelectedMapUnitIndex = -1;

    // 当前选中的出战图标（-1 = 无）
    int32 SelectedPartyIconIdx = -1;

    // 当前选中的备战图标（-1 = 无）
    int32 SelectedReserveIndex = -1;

    // 已部署的单位列表（索引对应 GameInstance->PartyUnits 索引）
    UPROPERTY()
    TArray<TObjectPtr<ALFPTacticsUnit>> DeployedUnits;

    // 单位 → PartyIndex 反向映射
    TMap<ALFPTacticsUnit*, int32> UnitToPartyIndex;

    // 缓存的玩家出生点格子
    TArray<ALFPHexTile*> PlayerSpawnTiles;

    // 当前选中高亮的格子（用于清除上一次高亮）
    UPROPERTY()
    TObjectPtr<ALFPHexTile> HighlightedSelectionTile;

};
