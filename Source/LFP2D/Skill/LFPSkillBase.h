// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "LFPSkillBase.generated.h"

class ALFPTacticsUnit;
class ALFPHexTile;
class ALFPTacticsPlayerController;

struct FLFPHexCoordinates;

UENUM(BlueprintType)
enum class ESkillTargetType : uint8
{
    Self,
    SingleAlly,
    SingleEnemy,
    AreaAlly,
    AreaEnemy,
    AreaAll,
    Tile
};

USTRUCT(BlueprintType)
struct FSkillRange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinRange = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRange = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequireLineOfSight = true;
};
/**
 *
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class LFP2D_API ULFPSkillBase : public UObject
{
	GENERATED_BODY()

public:
    ULFPSkillBase();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Skill")
    void Execute(ALFPHexTile* TargetTile = nullptr);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
    bool CanExecute(ALFPHexTile* TargetTile = nullptr);

    // 轻量检查：冷却和行动点是否满足（AI 选技能时用）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    bool IsAvailable() const;

    // 位置检查：从施法格子能否对目标格子释放（AI 找站位时用）
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
    bool CanReleaseFrom(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    FString GetCooldownStatus() const;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    TArray<ALFPHexTile*> GetTargetTiles(ALFPTacticsUnit* Caster) const;

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void OnTurnStart();

    // ==== 技能优先级（AI 规划阶段用于全局 AP 分配） ====

    // 释放后降低优先级
    UFUNCTION(BlueprintCallable, Category = "Skill|Priority")
    void OnSkillUsed();

    // 每轮回复优先级（在 OnTurnStart 中调用）
    UFUNCTION(BlueprintCallable, Category = "Skill|Priority")
    void RecoverPriority();

    // 条件提升（预留接口，子类/蓝图可覆盖）
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Skill|Priority")
    float EvaluateConditionBonus() const;

    // 获取本轮有效优先级（当前值 + 条件加成）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Priority")
    float GetEffectivePriority() const;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Skill")
    void UpdateSkillRange();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    TArray<FLFPHexCoordinates> GetReleaseRange() { return ReleaseRangeCoords; }

    // AI 目标选择：计算对目标的仇恨值（值越高，越优先攻击）
    // Caster：使用技能的敌方单位（自身属性/位置）
    // Target：候选的玩家单位（目标属性/位置）
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|AI")
    float CalculateHatredValue(ALFPTacticsUnit* Caster, ALFPTacticsUnit* Target) const;

    /*UFUNCTION(BlueprintCallable, Category = "Skill")
    void ShowReleaseRange(bool bShow = true);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void ShowEffectRange(bool bShow = true);*/
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ALFPTacticsUnit* Owner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ALFPTacticsPlayerController* OwnerController;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText SkillDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    UTexture2D* SkillIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 CooldownRounds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    int32 CurrentCooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillTargetType TargetType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FGameplayTagContainer SkillTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FSkillRange Range;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 ActionPointCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bIsDefaultAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FLFPHexCoordinates> ReleaseRangeCoords;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FLFPHexCoordinates> EffectRangeCoords;

    // ==== 技能优先级属性 ====

    // 当前运行时优先级
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Priority")
    float SkillPriority;

    // 基础优先级（设计值，也是上限）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Priority")
    float BasePriority = 50.0f;

    // 释放后优先级下降值
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Priority")
    float PriorityDecreaseOnUse = 30.0f;

    // 每轮优先级回复值
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Priority")
    float PriorityRecoveryPerRound = 10.0f;
};
