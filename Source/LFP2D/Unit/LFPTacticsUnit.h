// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFP2D/Unit/LFPUnitTypes.h"
#include "Components/TimelineComponent.h"
#include "PaperSpriteComponent.h"
#include "LFPTacticsUnit.generated.h"

// 委托签名
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, int32, CurrentHealth, int32, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitDeathSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoveFinishedSignature);

// EUnitAffiliation 定义在 LFPBattleTypes.h 中
// ELFPUnitRace 和 ELFPAttackType 定义在 LFPUnitTypes.h 中

class ULFPSkillBase;
class ALFPHexGridManager;
class ALFPTurnManager;
class ULFPUnitRegistryDataAsset;
struct FLFPUnitRegistryEntry;
class ULFPBetrayalCondition;
class ALFPAIController;
class ULFPSkillComponent;

USTRUCT(BlueprintType)
struct FLFPUnitBaseStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    ELFPAttackType AttackType = ELFPAttackType::AT_Physical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Attack = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MaxHealth = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MaxMovePoints = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Speed = 5;
};

USTRUCT(BlueprintType)
struct FLFPUnitAdvancedStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Advanced Stats")
    int32 AttackCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Advanced Stats")
    int32 ActionCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Advanced Stats")
    int32 PhysicalBlock = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Advanced Stats")
    int32 SpellDefense = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Advanced Stats")
    int32 Weight = 0;
};

UCLASS()
class LFP2D_API ALFPTacticsUnit : public APawn
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

    // 可移动范围缓存
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    TArray<ALFPHexTile*> MovementRangeTiles;

    // 移动到目标格子
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    bool MoveToTile(ALFPHexTile* NewTargetTile);

    // 设置选中状态
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetSelected(bool bSelected);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool IsSelected() const { return bIsSelected; }

    // 获取移动范围
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetMovementRange() const { return GetCurrentMovePoints(); }

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetBaseMovePoints() const { return BaseMaxMovePoints; }

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetCurrentMovePoints() const { return CurrentMovePoints; }

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetCurrentMaxMovePoints() const { return CurrentMaxMovePoints; }

    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    int32 GetMaxMovePoints() const { return GetCurrentMaxMovePoints(); }

    // 消耗移动力
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void ConsumeMovePoints(int32 Amount);

	// 消耗行动力
	UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
	void ConsumeActionPoints(int32 Amount);

    // 检查是否有足够移动力
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughMovePoints(int32 Required) const;

    // 检查是否有足够行动力
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughActionPoints(int32 Required) const;

    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    int32 GetActionPoints();

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    bool InitializeFromRegistry(ULFPUnitRegistryDataAsset* Registry);

    void ApplyRegistryEntry(const FLFPUnitRegistryEntry& Entry);

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    void ResetCurrentStatsToBase(bool bResetHealth = true);

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    void AddCurrentAttack(int32 Delta);

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    void AddCurrentMaxHealth(int32 Delta, bool bKeepHealthRatio = false);

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    void AddCurrentSpeed(int32 Delta);

    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    void AddCurrentPhysicalBlock(int32 Delta);

    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    ALFPHexGridManager* GetGridManager() const;

    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
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

    // 吸附到网格
    void SnapToGrid();

protected:
    // 旧移动范围字段已废弃，当前移动范围由当前最大/剩余移动力驱动
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MovementRange = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Base")
    ELFPAttackType BaseAttackType = ELFPAttackType::AT_Physical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Base")
    int32 BaseAttack = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Base")
    int32 BaseMaxHealth = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Base")
    int32 BaseMaxMovePoints = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Base")
    int32 BaseSpeed = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Advanced")
    int32 BaseAttackCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Advanced")
    int32 BaseActionCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Advanced")
    int32 BasePhysicalBlock = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Advanced")
    int32 BaseSpellDefense = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Advanced")
    int32 BaseWeight = 0;

    // 当前运行时属性
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Stats")
    ELFPAttackType CurrentAttackType = ELFPAttackType::AT_Physical;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Stats")
    int32 CurrentAttack = 10;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Stats")
    int32 CurrentMaxHealth = 100;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Stats")
    int32 CurrentMaxMovePoints = 3;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Stats")
    int32 CurrentMovePoints = 3;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Stats")
    int32 CurrentSpeed = 5;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Advanced")
    int32 CurrentAttackCount = 1;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Advanced")
    int32 CurrentActionCount = 1;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Advanced")
    int32 CurrentPhysicalBlock = 5;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Advanced")
    int32 CurrentSpellDefense = 0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Unit State|Advanced")
    int32 CurrentWeight = 0;

    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    bool bStatsInitialized = false;

    // MaxActionPoints 和 CurrentActionPoints 已迁移至 TurnManager 的阵营 AP 系统

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

public:
    // 是否正在移动动画中
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving = false;

    // 移动速度（格/秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 5.0f;

    // 移动完成委托
    UPROPERTY(BlueprintAssignable, Category = "Movement")
    FOnMoveFinishedSignature OnMoveFinished;

public:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UPaperSpriteComponent* SpriteComponent;

public:
    // 回合系统相关
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void ResetForNewRound();

    UFUNCTION(BlueprintPure, Category = "Turn System")
    bool CanAct() const { return !bHasActed && bOnTurn; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetBaseSpeed() const { return BaseSpeed; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetCurrentSpeed() const { return CurrentSpeed; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetSpeed() const { return GetCurrentSpeed(); }

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
    int32 Speed = 5; // 旧字段保留兼容，运行时请改用 CurrentSpeed

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

	// 在单位上广播血量变化事件
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

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetBaseMaxHealth() const { return BaseMaxHealth; }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetCurrentMaxHealth() const { return CurrentMaxHealth; }

	// 获取最大血量
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
	int32 GetMaxHealth() const { return GetCurrentMaxHealth(); }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    ELFPAttackType GetAttackType() const { return CurrentAttackType; }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetBaseAttack() const { return BaseAttack; }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetCurrentAttack() const { return CurrentAttack; }

    // 获取攻击力
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetAttackPower() const { return GetCurrentAttack(); }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetAttackCount() const { return CurrentAttackCount; }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetActionCount() const { return CurrentActionCount; }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetPhysicalBlock() const { return CurrentPhysicalBlock; }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetSpellDefense() const { return CurrentSpellDefense; }

    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    int32 GetWeight() const { return CurrentWeight; }

	// 获取阵营标识
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
    EUnitAffiliation GetAffiliation() const { return Affiliation; }

    UFUNCTION(BlueprintPure, Category = "Unit Identity")
    ELFPUnitRace GetUnitRace() const { return UnitRace; }

    UFUNCTION(BlueprintPure, Category = "Unit Identity")
    const FGameplayTagContainer& GetSpecialTags() const { return SpecialTags; }

    UFUNCTION(BlueprintPure, Category = "Unit Identity")
    bool HasSpecialTag(FGameplayTag Tag) const { return SpecialTags.HasTag(Tag); }

    UFUNCTION(BlueprintPure, Category = "Unit Identity")
    bool HasAnySpecialTags(const FGameplayTagContainer& InTags) const { return SpecialTags.HasAny(InTags); }

	// 血条组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* HealthBarComponent;

	// 初始化血条
	UFUNCTION(BlueprintCallable, Category = "Unit Display")
	void InitializeHealthBar();

public:
    // 血量属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MaxHealth = 100; // 旧字段保留兼容，运行时请改用 CurrentMaxHealth

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit Stats")
    int32 CurrentHealth = 100;

    // 攻击力属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 AttackPower = 10; // 旧字段保留兼容，运行时请改用 CurrentAttack

    // 最大移动力属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MaxMovePoints = 3; // 旧字段保留兼容，运行时请改用 CurrentMaxMovePoints

    // 防御力属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Defense = 5; // 旧字段保留兼容，运行时请改用 CurrentPhysicalBlock

    // ==== 击杀掉落 ====

    // 击杀时掉落金币
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Loot")
    int32 DropGold = 0;

    // 击杀时掉落食物
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats|Loot")
    int32 DropFood = 0;

    // 阵营标识
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    EUnitAffiliation Affiliation = EUnitAffiliation::UA_Player;

    // ==== 单位身份 ====

    // 单位类型 ID（对应 ULFPUnitRegistryDataAsset 中的键）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Identity")
    FName UnitTypeID = NAME_None;

    // 单位阶级
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Unit Identity")
    int32 UnitTier = 1;

    // 单位种族
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Unit Identity")
    ELFPUnitRace UnitRace = ELFPUnitRace::UR_None;

    // 特殊标签
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Unit Identity")
    FGameplayTagContainer SpecialTags;

    // 生成轻量单位数据（用于捕获、保存）
    FLFPUnitEntry ToUnitEntry() const
    {
        FLFPUnitEntry Entry;
        Entry.TypeID = UnitTypeID;
        Entry.Tier = UnitTier;
        return Entry;
    }

    // ==== 战斗相关 ====

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

    // 受伤处理
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnTakeDamage(int32 DamageTaken);

    // 治疗处理
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnHeal(int32 HealAmount);

    // 播放攻击动画
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void PlayAttackAnimation(ALFPTacticsUnit* Target);

    // 获取攻击范围
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    TArray<ALFPHexTile*> GetAttackRangeTiles();

    // 获取攻击范围大小
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    int32 GetAttackRange() { return AttackRange; }

    // 检查目标是否在攻击范围内
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsTargetInAttackRange(ALFPTacticsUnit* Target) const;

    // 阵营颜色
    UFUNCTION(BlueprintPure, Category = "Unit Display")
    FLinearColor GetAffiliationColor() const;

    void UpdateHealthUI();

protected:
    // 处理死亡
    void HandleDeath();

    // 攻击范围
    UPROPERTY(EditAnywhere, Category = "Unit Combat")
    int32 AttackRange = 2;

    // 近战攻击模式
    UPROPERTY(EditAnywhere, Category = "Unit Combat")
    bool bMeleeAttack = true;

    // 死亡状态
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State", meta = (AllowPrivateAccess = "true"))
    bool bIsDead = false;

public:
    UFUNCTION(BlueprintCallable, Category = "Betrayal")
    void ChangeAffiliation(EUnitAffiliation NewAffiliation = EUnitAffiliation::UA_Player);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Betrayal")
	TArray<ULFPBetrayalCondition*> BetrayalConditions;

    // 敌方AI控制器支持和事件
public:
    // 获取AI控制器
    UFUNCTION(BlueprintPure, Category = "AI")
    ALFPAIController* GetAIController() const { return AIController; }

    //// 事件：移动完成
    //UPROPERTY(BlueprintAssignable, Category = "Events")
    //FSimpleMulticastDelegate OnMoveCompleteDelegate;

    //// 事件：攻击完成
    //UPROPERTY(BlueprintAssignable, Category = "Events")
    //FSimpleMulticastDelegate OnAttackCompleteDelegate;

    //// 在移动完成后触发事件
    //UFUNCTION()
    //void NotifyMoveComplete()
    //{
    //    OnMoveCompleteDelegate.Broadcast();
    //}

    //// 在攻击完成后触发事件
    //UFUNCTION()
    //void NotifyAttackComplete()
    //{
    //    OnAttackCompleteDelegate.Broadcast();
    //}

protected:
    // AI控制器
    UPROPERTY()
    ALFPAIController* AIController;

public:
    // 寻找最佳目标
    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual ALFPTacticsUnit* FindBestTarget();

    // 寻找最佳移动位置
    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual ALFPHexTile* FindBestMovementTile(ALFPTacticsUnit* Target);

    // 计算威胁值
    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual float CalculateThreatValue(ALFPTacticsUnit* Target);

    // 计算位置价值
    UFUNCTION(BlueprintCallable, Category = "AI")
    virtual float CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target);

public:
    TArray<ULFPSkillBase*> GetAvailableSkills();

    bool ExecuteSkill(ULFPSkillBase* Skill, ALFPHexTile* NewTargetTile = nullptr);

    ULFPSkillBase* GetDefaultAttackSkill();

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<ULFPSkillComponent> SkillComponent;

    // ==== 行动计划（敌人规划阶段使用） ====
public:
    // 当前行动计划
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
    FEnemyActionPlan CurrentActionPlan;

    // 设置行动计划并显示头顶图标
    UFUNCTION(BlueprintCallable, Category = "AI Plan")
    void SetActionPlan(const FEnemyActionPlan& Plan);

    // 清除行动计划并隐藏图标
    UFUNCTION(BlueprintCallable, Category = "AI Plan")
    void ClearActionPlan();

    // 是否有有效的行动计划
    UFUNCTION(BlueprintPure, Category = "AI Plan")
    bool HasActionPlan() const { return CurrentActionPlan.bIsValid; }

    // 显示/隐藏头顶技能图标
    UFUNCTION(BlueprintCallable, Category = "AI Plan")
    void ShowPlannedSkillIcon(bool bShow);

protected:
    // 头顶计划技能图标组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UWidgetComponent* PlannedSkillIconComponent;

    // 技能图标 Widget 蓝图类
    UPROPERTY(EditDefaultsOnly, Category = "AI Plan")
    TSubclassOf<class ULFPPlannedSkillIconWidget> PlannedSkillIconWidgetClass;

	// 技能图标距头顶距离
	UPROPERTY(EditDefaultsOnly, Category = "AI Plan")
    float SkillIconTopDist = 100.f;
};
