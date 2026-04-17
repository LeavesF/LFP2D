// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "UObject/NoExportTypes.h"
#include "LFPSkillBase.generated.h"

class ALFPTacticsUnit;
class ULFPSkillRangeDataAsset;
struct FPropertyChangedEvent;

UENUM(BlueprintType)
enum class ESkillTargetType : uint8
{
	Self,
	SingleAlly,
	SingleEnemy,
    SingleUnit,
    MutiAlly,
    MutiEnemy,
    MutiUnit,
	AllAlly,
	AllEnemy,
	AllUnit,
    AnyTile
};

UENUM(BlueprintType)
enum class ESkillRangeType : uint8
{
	Origin,
	Coverage,
	Ring,
	Ray,
	Custom
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

	// 创建后初始化（蓝图默认值已应用后调用，初始化运行时状态）
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void InitSkill(ALFPTacticsUnit* InOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void Execute(ALFPHexTile* TargetTile = nullptr);
	virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
	ALFPTacticsUnit* GetUnitOnTile(ALFPHexTile* Tile) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
	bool IsHostileTarget(ALFPTacticsUnit* Target) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
	bool IsValidReleaseTargetTile(ALFPHexTile* TargetTile) const;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	int32 DealOwnerRepeatedDamage(ALFPTacticsUnit* Target, int32 HitCount, float DamageScalePerHit) const;

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
	virtual void OnTurnStart();

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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Range")
	void UpdateSkillRange();
	virtual void UpdateSkillRange_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Range")
	TArray<FLFPHexCoordinates> GetReleaseRange() const { return ReleaseRangeCoords; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Range")
	FString GetReleaseRangeDescription() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Range")
	TArray<FLFPHexCoordinates> GetEffectRange() const { return EffectRangeCoords; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Range")
	FString GetEffectRangeDescription() const;

	UFUNCTION(BlueprintCallable, Category = "Skill|Custom Range")
	bool ApplyCustomReleaseRangePreset();

	UFUNCTION(BlueprintCallable, Category = "Skill|Custom Range")
	bool ApplyCustomEffectRangePreset();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Custom Range")
	TArray<FName> GetAvailableCustomReleaseRangePresetNames() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Custom Range")
	TArray<FName> GetAvailableCustomEffectRangePresetNames() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Custom Range")
	TArray<FString> GetAvailableCustomReleaseRangePresetOptions() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill|Custom Range")
	TArray<FString> GetAvailableCustomEffectRangePresetOptions() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Range")
	TArray<FLFPHexCoordinates> GetReleaseRangeInGrid();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Range")
    TArray<FLFPHexCoordinates> GetEffectRangeInGrid();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range")
	ESkillRangeType ReleaseRangeType = ESkillRangeType::Coverage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range", meta = (ClampMin = "0"))
	int32 MinRange = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range", meta = (ClampMin = "0"))
	int32 MaxRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range", meta = (ClampMin = "0"))
	int32 RayRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Custom Range", meta = (EditCondition = "ReleaseRangeType == ESkillRangeType::Custom"))
	TObjectPtr<ULFPSkillRangeDataAsset> CustomReleaseRangeData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Custom Range", meta = (EditCondition = "ReleaseRangeType == ESkillRangeType::Custom", GetOptions = "GetAvailableCustomReleaseRangePresetOptions"))
	FName CustomReleaseRangePresetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 ActionPointCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	bool bIsDefaultAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range")
	TArray<FLFPHexCoordinates> ReleaseRangeCoords;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Skill|Range")
	TArray<FLFPHexCoordinates> ReleaseRangeInGridCoords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range")
	TArray<FLFPHexCoordinates> EffectRangeCoords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range")
	ESkillRangeType EffectRangeType = ESkillRangeType::Custom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range", meta = (ClampMin = "0"))
	int32 EffectMinRange = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range", meta = (ClampMin = "0"))
	int32 EffectMaxRange = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range", meta = (ClampMin = "0"))
	int32 EffectRayRange = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Custom Range", meta = (EditCondition = "EffectRangeType == ESkillRangeType::Custom"))
	TObjectPtr<ULFPSkillRangeDataAsset> CustomEffectRangeData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Custom Range", meta = (EditCondition = "EffectRangeType == ESkillRangeType::Custom", GetOptions = "GetAvailableCustomEffectRangePresetOptions"))
	FName CustomEffectRangePresetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Range")
	TArray<FLFPHexCoordinates> EffectRangeInGridCoords;

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

private:
	void RebuildReleaseRangeCoords();
	void RebuildEffectRangeCoords();
	bool HasReleaseCoord(const FLFPHexCoordinates& RelativeCoord) const;
	bool TryApplyCustomReleaseRangePreset();
	bool TryApplyCustomEffectRangePreset();
	void RebuildRangeCoords(
		TArray<FLFPHexCoordinates>& RangeCoords,
		ESkillRangeType RangeType,
		int32 RangeMin,
		int32 RangeMax,
		int32 RangeRay,
		ULFPSkillRangeDataAsset* CustomRangeData,
		FName CustomRangePresetName);
	bool TryApplyCustomRangePreset(
		TArray<FLFPHexCoordinates>& RangeCoords,
		ESkillRangeType RangeType,
		ULFPSkillRangeDataAsset* CustomRangeData,
		FName CustomRangePresetName);
	FString GetRangeDescription(
		ESkillRangeType RangeType,
		int32 RangeMin,
		int32 RangeMax,
		int32 RangeRay,
		ULFPSkillRangeDataAsset* CustomRangeData,
		FName CustomRangePresetName) const;
	TArray<FName> GetAvailableCustomRangePresetNames(ULFPSkillRangeDataAsset* CustomRangeData) const;
	TArray<FString> GetAvailableCustomRangePresetOptions(ULFPSkillRangeDataAsset* CustomRangeData) const;

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
