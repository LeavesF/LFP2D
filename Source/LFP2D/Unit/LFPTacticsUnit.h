// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "Components/TimelineComponent.h"
#include "PaperSpriteComponent.h"
#include "LFPTacticsUnit.generated.h"

// 委托签名
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, int32, CurrentHealth, int32, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitDeathSignature);

// 单位阵营枚举
UENUM(BlueprintType)
enum class EUnitAffiliation : uint8
{
    UA_Player     UMETA(DisplayName = "Player"),
    UA_Enemy      UMETA(DisplayName = "Enemy"),
    UA_Neutral    UMETA(DisplayName = "Neutral")
};

class ALFPHexGridManager;
class ALFPTurnManager;

UCLASS()
class LFP2D_API ALFPTacticsUnit : public AActor
{
	GENERATED_BODY()
	
public:
    ALFPTacticsUnit();

    // 设置/获取当前坐标
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    FLFPHexCoordinates GetCurrentCoordinates() const { return CurrentCoordinates; }

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    ALFPHexTile* GetCurrentTile();
    //// 获取可移动范围
    //UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    //TArray<ALFPHexTile*> GetMovementRangeTiles();

    // 可移动范围格子
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    TArray<ALFPHexTile*> MovementRangeTiles;

    // 移动到目标格子
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    bool MoveToTile(ALFPHexTile* TargetTile);

    // 设置选中状态
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetSelected(bool bSelected);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool IsSelected() const { return bIsSelected; }

    // 获取移动范围
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetMovementRange() const { return CurrentMovePoints; }

    // 消耗移动点
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void ConsumeMovePoints(int32 Amount);

	// 消耗行动点
	UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
	void ConsumeActionPoints(int32 Amount);

    // 检查是否有足够移动点
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughMovePoints(int32 Required) const;

    // 检查是否有足够行动点
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughActionPoints(int32 Required) const;

    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    int32 GetMovePoints(int32 Amount) { return CurrentMovePoints; }

	UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
	int32 GetActionPoints(int32 Amount) { return CurrentActionPoints; }

    ALFPHexGridManager* GetGridManager() const;
    ALFPTurnManager* GetTurnManager() const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // 移动动画更新
    UFUNCTION()
    void UpdateMoveAnimation(float Value);

    // 完成移动
    void FinishMove();

    // 对齐格子
    void SnapToGrid();

protected:
    // 单位属性
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MovementRange = 5;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MaxMovePoints = 3;

    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    int32 CurrentMovePoints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
	int32 MaxActionPoints = 3;

	UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
	int32 CurrentActionPoints;

    // 当前坐标
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_Q = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_R = 0;

    FLFPHexCoordinates CurrentCoordinates;

    // 选中状态
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    bool bIsSelected = false;

    // 移动状态
    UPROPERTY(Transient)
    ALFPHexTile* TargetTile;

    TArray<ALFPHexTile*> MovePath;
    int32 CurrentPathIndex;
    float MoveProgress;

    // 动画
    FTimerHandle MoveTimerHandle;
    FTimeline MoveTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UCurveFloat* MoveCurve;

    // 组件
    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UPaperSpriteComponent* SpriteComponent;

public:
    // 回合系统函数
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void ResetForNewRound();

    UFUNCTION(BlueprintPure, Category = "Turn System")
    bool CanAct() const { return !bHasActed && bOnTurn; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetSpeed() const { return Speed; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    bool HasActed() const { return bHasActed; }

    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void SetHasActed(bool bActed) { bHasActed = bActed; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn System")
    UTexture2D* GetUnitIcon() const { return UnitIconTexture; }

    // 回合事件
    UFUNCTION()
    void OnTurnStarted();

    UFUNCTION()
    void OnTurnEnded();

public:
    // 回合系统属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Speed = 5; // 速度值（决定行动顺序）

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bHasActed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bOnTurn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn System")
    UTexture2D* UnitIconTexture;

public:
    UFUNCTION(BlueprintCallable, Category = "Mouse Input")
    void OnMouseEnter();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mouse Input")
    void OnMouseExit();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mouse Input")
    void OnMouseClick();

	// 在单位类中添加血量变化事件
public:
	// 事件：血量变化
	UPROPERTY(BlueprintAssignable, Category = "Unit Events")
	FOnHealthChangedSignature OnHealthChangedDelegate;

	// 事件：单位死亡
	UPROPERTY(BlueprintAssignable, Category = "Unit Events")
    FOnUnitDeathSignature OnDeathDelegate;

	// 获取当前血量
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
	int32 GetCurrentHealth() const { return CurrentHealth; }

	// 获取最大血量
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
	int32 GetMaxHealth() const { return MaxHealth; }

	// 获取阵营标识
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
    EUnitAffiliation GetAffiliation() const { return Affiliation; }

	// 血条组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* HealthBarComponent;

	// 初始化血条
	UFUNCTION(BlueprintCallable, Category = "Unit Display")
	void InitializeHealthBar();

public:
    // 血量属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MaxHealth = 100;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit Stats")
    int32 CurrentHealth = 100;

    // 攻击力属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 AttackPower = 10;

    // 防御力属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Defense = 5;

    // 阵营标识
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    EUnitAffiliation Affiliation = EUnitAffiliation::UA_Player;

    // ==== 新增函数 ====

    // 应用伤害
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    void TakeDamage(int32 Damage);

    // 治疗单位
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    void Heal(int32 Amount);

    // 检查是否存活
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsAlive() const { return CurrentHealth > 0; }

    // 检查是否为敌方单位
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsEnemy() const { return Affiliation == EUnitAffiliation::UA_Enemy; }

    // 检查是否为友方单位
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsAlly() const { return Affiliation == EUnitAffiliation::UA_Player; }

    // 检查是否为中立单位
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsNeutral() const { return Affiliation == EUnitAffiliation::UA_Neutral; }

    // 攻击目标单位
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    bool AttackTarget(ALFPTacticsUnit* Target);

    void ApplyDamageToTarget(ALFPTacticsUnit* Target);

    // 死亡处理
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnDeath();

    // 伤害处理
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnTakeDamage(int32 DamageTaken);

    // 治疗处理
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnHeal(int32 HealAmount);

    // 攻击动画
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void PlayAttackAnimation(ALFPTacticsUnit* Target);

    // 获取攻击范围
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    TArray<ALFPHexTile*> GetAttackRangeTiles();

    // 检查目标是否在攻击范围内
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsTargetInAttackRange(ALFPTacticsUnit* Target) const;

    // 阵营颜色
    UFUNCTION(BlueprintPure, Category = "Unit Display")
    FLinearColor GetAffiliationColor() const;

    void UpdateHealthUI();

protected:
    // 死亡处理
    void HandleDeath();

    // 攻击范围
    UPROPERTY(EditAnywhere, Category = "Unit Combat")
    int32 AttackRange = 2;

    // 攻击范围模式
    UPROPERTY(EditAnywhere, Category = "Unit Combat")
    bool bMeleeAttack = true;

    // 死亡状态
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State", meta = (AllowPrivateAccess = "true"))
    bool bIsDead = false;
};
