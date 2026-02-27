// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LFPBattleTypes.generated.h"

class ALFPTacticsUnit;
class ULFPSkillBase;
class ALFPHexTile;

// 单位阵营枚举（从 LFPTacticsUnit.h 迁移至此，供多个系统使用）
UENUM(BlueprintType)
enum class EUnitAffiliation : uint8
{
	UA_Player     UMETA(DisplayName = "Player"),
	UA_Enemy      UMETA(DisplayName = "Enemy"),
	UA_Neutral    UMETA(DisplayName = "Neutral")
};

// 战斗阶段枚举
UENUM(BlueprintType)
enum class EBattlePhase : uint8
{
	BP_EnemyPlanning   UMETA(DisplayName = "Enemy Planning"),
	BP_ActionPhase     UMETA(DisplayName = "Action Phase"),
	BP_RoundEnd        UMETA(DisplayName = "Round End")
};

// 敌人行动计划结构体
USTRUCT(BlueprintType)
struct FEnemyActionPlan
{
	GENERATED_BODY()

	// 执行计划的敌方单位
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
	ALFPTacticsUnit* EnemyUnit = nullptr;

	// 选定的技能
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
	ULFPSkillBase* PlannedSkill = nullptr;

	// 目标单位（可能在行动前死亡）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
	ALFPTacticsUnit* TargetUnit = nullptr;

	// 技能瞄准的格子（= 目标单位规划时所在格子）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
	ALFPHexTile* TargetTile = nullptr;

	// 敌人移动到的施法位置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
	ALFPHexTile* CasterPositionTile = nullptr;

	// 技能生效范围内的格子（绝对位置）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
	TArray<ALFPHexTile*> EffectAreaTiles;

	// 计划是否有效
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Plan")
	bool bIsValid = false;

	void Reset()
	{
		EnemyUnit = nullptr;
		PlannedSkill = nullptr;
		TargetUnit = nullptr;
		TargetTile = nullptr;
		CasterPositionTile = nullptr;
		EffectAreaTiles.Empty();
		bIsValid = false;
	}
};
