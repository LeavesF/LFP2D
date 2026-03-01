// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFPTurnManager.generated.h"

class ALFPTacticsUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, EBattlePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFactionAPChangedSignature, EUnitAffiliation, Faction, int32, NewAP);

UCLASS()
class LFP2D_API ALFPTurnManager : public AActor
{
	GENERATED_BODY()

public:
    ALFPTurnManager();

    // 开始游戏回合系统
    void StartGame();

    // 开始新回合
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void BeginNewRound();

    // 结束当前回合
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void EndCurrentRound();

    // 获取当前行动单位
    UFUNCTION(BlueprintPure, Category = "Turn System")
    ALFPTacticsUnit* GetCurrentUnit() const { return CurrentUnit; }

    // 获取当前回合数
    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetCurrentRound() const { return CurrentRound; }

    // 获取当前单位列表
    UFUNCTION(BlueprintPure, Category = "Turn System")
    TArray<ALFPTacticsUnit*> GetTurnOrderUnits() const { return TurnOrderUnits; }

    // 传递回合到下一个单位
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void PassTurn();

    // 当单位完成行动
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void OnUnitFinishedAction(ALFPTacticsUnit* Unit);

    // 注册单位到回合系统
    void RegisterUnit(ALFPTacticsUnit* Unit);

    // 从回合系统注销单位
    void UnregisterUnit(ALFPTacticsUnit* Unit);

    // ==== 阶段系统 ====

    // 获取当前战斗阶段
    UFUNCTION(BlueprintPure, Category = "Turn System")
    EBattlePhase GetCurrentPhase() const { return CurrentPhase; }

    // 查询某敌人的行动计划
    UFUNCTION(BlueprintPure, Category = "Turn System")
    const FEnemyActionPlan& GetPlanForEnemy(ALFPTacticsUnit* Enemy) const;

    // 获取所有敌人计划
    UFUNCTION(BlueprintPure, Category = "Turn System")
    const TArray<FEnemyActionPlan>& GetAllEnemyPlans() const { return EnemyActionPlans; }

    // ==== 阵营行动点 ====

    UFUNCTION(BlueprintPure, Category = "Action Points")
    int32 GetFactionAP(EUnitAffiliation Faction) const;

    UFUNCTION(BlueprintPure, Category = "Action Points")
    bool HasEnoughFactionAP(EUnitAffiliation Faction, int32 Amount) const;

    UFUNCTION(BlueprintCallable, Category = "Action Points")
    void ConsumeFactionAP(EUnitAffiliation Faction, int32 Amount);

public:
    UPROPERTY(BlueprintAssignable, Category = "Turn System")
    FOnTurnChangedSignature OnTurnChanged;

    UPROPERTY(BlueprintAssignable, Category = "Turn System")
    FOnPhaseChangedSignature OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Action Points")
    FOnFactionAPChangedSignature OnFactionAPChanged;

    UFUNCTION(BlueprintPure, Category = "Action Points")
    int32 GetFactionMaxAP() const { return FactionMaxAP; }

protected:
    virtual void BeginPlay() override;

protected:
    // 按速度排序单位
    void SortUnitsBySpeed();

    // 开始单位回合
    void BeginUnitTurn(ALFPTacticsUnit* Unit);

    // 结束单位回合
    void EndUnitTurn(ALFPTacticsUnit* Unit);

    // 设置阶段并广播
    void SetPhase(EBattlePhase NewPhase);

    // ==== 敌人规划阶段 ====

    // 开始敌人规划阶段
    void BeginEnemyPlanningPhase();

    // Step 1: 全局技能分配（按优先级分配AP技能）
    void AllocateEnemySkills();

    // 处理下一个敌人的规划
    void ProcessNextEnemyPlan();

    // 结束敌人规划阶段
    void EndEnemyPlanningPhase();

    // ==== 行动阶段 ====

    // 开始行动阶段
    void BeginActionPhase();

    // 执行敌人的预定计划
    void ExecuteEnemyPlan(ALFPTacticsUnit* Unit);

protected:
    // 单位列表
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    TArray<ALFPTacticsUnit*> TurnOrderUnits;

    // 当前行动单位
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    ALFPTacticsUnit* CurrentUnit;

    // 当前回合数
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    int32 CurrentRound = 0;

    // 是否在回合中
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    bool bIsInRound = false;

    // 当前战斗阶段
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    EBattlePhase CurrentPhase = EBattlePhase::BP_RoundEnd;

    // 本轮所有敌人行动计划
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    TArray<FEnemyActionPlan> EnemyActionPlans;

    // 规划阶段的敌人排序
    UPROPERTY()
    TArray<ALFPTacticsUnit*> PlanningOrderEnemies;

    // 当前正在规划的敌人索引
    int32 CurrentPlanningEnemyIndex = 0;

    // 技能分配结果（Step1 → Step2 传递）
    UPROPERTY()
    TMap<ALFPTacticsUnit*, ULFPSkillBase*> AllocatedSkills;

    // ==== 阵营行动点配置 ====

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Points")
    int32 FactionMaxAP = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Points")
    int32 FactionInitialAP = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Points")
    int32 FactionAPRecovery = 1;

    // 当前阵营 AP
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action Points")
    TMap<EUnitAffiliation, int32> FactionCurrentAP;

    // 空计划（用于查询未找到时返回）
    static FEnemyActionPlan EmptyPlan;
};
