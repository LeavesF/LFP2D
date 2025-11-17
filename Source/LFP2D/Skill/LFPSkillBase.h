// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "LFPSkillBase.generated.h"

class ALFPTacticsUnit;
class ALFPHexTile;

UENUM(BlueprintType)
enum class ESkillTargetType : uint8
{
    Self,           // 对自己施放
    SingleAlly,     // 单个友方
    SingleEnemy,    // 单个敌方
    AreaAlly,       // 区域友方
    AreaEnemy,      // 区域敌方
    AreaAll,        // 区域所有单位
    Tile            // 特定格子
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

    // 技能执行
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void Execute(ALFPTacticsUnit* Caster, ALFPHexTile* TargetTile = nullptr);

    // 检查技能是否可用
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool CanExecute(ALFPTacticsUnit* Caster) const;

    // 获取技能冷却状态
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    FString GetCooldownStatus() const;

    // 获取技能范围内的所有目标格子
    UFUNCTION(BlueprintCallable, Category = "Skill")
    TArray<ALFPHexTile*> GetTargetTiles(ALFPTacticsUnit* Caster) const;

    // 回合开始时的冷却更新
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void OnTurnStart();

    // 技能属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText SkillDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    UTexture2D* SkillIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 CooldownRounds; // 冷却回合数

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    int32 CurrentCooldown; // 当前冷却计数

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillTargetType TargetType;

    // 技能标签，用于分类和过滤
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FGameplayTagContainer SkillTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FSkillRange Range;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 ActionPointCost; // 行动点消耗

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bIsDefaultAttack; // 是否为默认攻击技能
};
