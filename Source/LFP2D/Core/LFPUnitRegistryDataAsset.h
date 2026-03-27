#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPUnitRegistryDataAsset.generated.h"

class ALFPTacticsUnit;

// 单位注册表条目
USTRUCT(BlueprintType)
struct FLFPUnitRegistryEntry
{
	GENERATED_BODY()

	// 单位蓝图类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	TSubclassOf<ALFPTacticsUnit> UnitClass;

	// 显示名称
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	FText DisplayName;

	// 单位图标（UI 用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 该单位的阶级（用于 UI 显示，如 1/2/3）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	int32 Tier = 1;

	// 单位种族（唯一标签）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Identity")
	ELFPUnitRace Race = ELFPUnitRace::UR_None;

	// 特殊标签（可自由配置多个）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Identity")
	FGameplayTagContainer SpecialTags;

	// 基础属性模板
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Stats")
	FLFPUnitBaseStats BaseStats;

	// 高级属性模板
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Stats")
	FLFPUnitAdvancedStats AdvancedStats;

	// 可进化的目标 TypeID 列表（空=终阶，多个=分支选择）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Evolution")
	TArray<FName> EvolutionTargets;
};

/**
 * 单位注册表：全局唯一，映射 TypeID → 单位蓝图类和元数据
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPUnitRegistryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// TypeID → 单位数据映射
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	TMap<FName, FLFPUnitRegistryEntry> UnitRegistry;

	// 查找条目
	UFUNCTION(BlueprintPure, Category = "Unit Registry")
	bool FindEntry(FName TypeID, FLFPUnitRegistryEntry& OutEntry) const;

	// 获取蓝图类
	UFUNCTION(BlueprintPure, Category = "Unit Registry")
	TSubclassOf<ALFPTacticsUnit> GetUnitClass(FName TypeID) const;

	// 获取指定 TypeID 的进化目标列表
	UFUNCTION(BlueprintPure, Category = "Unit Registry|Evolution")
	TArray<FName> GetEvolutionTargets(FName TypeID) const;

	// 是否可进化（有进化目标）
	UFUNCTION(BlueprintPure, Category = "Unit Registry|Evolution")
	bool CanEvolve(FName TypeID) const;

	// 获取阶级
	UFUNCTION(BlueprintPure, Category = "Unit Registry")
	int32 GetUnitTier(FName TypeID) const;
};
