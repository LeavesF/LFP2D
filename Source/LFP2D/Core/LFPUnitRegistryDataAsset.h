#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPUnitRegistryDataAsset.generated.h"

class ALFPTacticsUnit;
class ULFPSkillBase;

/* 单位注册表条目：映射 TypeID 到单位蓝图、属性和默认携带卡。 */
USTRUCT(BlueprintType)
struct FLFPUnitRegistryEntry
{
	GENERATED_BODY()

	/* 单位蓝图类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	TSubclassOf<ALFPTacticsUnit> UnitClass;

	/* 显示名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	FText DisplayName;

	/* 单位图标，主要供 UI 使用。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/* 单位阶级，用于 UI 显示。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	int32 Tier = 1;

	/* 单位种族。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Identity")
	ELFPUnitRace Race = ELFPUnitRace::UR_None;

	/* 特殊标签，可配置多个。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Identity")
	FGameplayTagContainer SpecialTags;

	/* 基础属性模板。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Stats")
	FLFPUnitBaseStats BaseStats;

	/* 高级属性模板。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Stats")
	FLFPUnitAdvancedStats AdvancedStats;

	/* 可进化目标 TypeID 列表，空表示终阶。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Evolution")
	TArray<FName> EvolutionTargets;

	/* 单位类型默认携带进战斗牌库的卡牌技能类；每个出战实例仍会额外获得普通攻击卡。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry|Cards")
	TArray<TSubclassOf<ULFPSkillBase>> DefaultCarriedCardSkillClasses;
};

/* 单位注册表：全局唯一，映射 TypeID 到单位蓝图类和元数据。 */
UCLASS(Blueprintable)
class LFP2D_API ULFPUnitRegistryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Registry")
	TMap<FName, FLFPUnitRegistryEntry> UnitRegistry;

	UFUNCTION(BlueprintPure, Category = "Unit Registry")
	bool FindEntry(FName TypeID, FLFPUnitRegistryEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Unit Registry")
	TSubclassOf<ALFPTacticsUnit> GetUnitClass(FName TypeID) const;

	UFUNCTION(BlueprintPure, Category = "Unit Registry|Evolution")
	TArray<FName> GetEvolutionTargets(FName TypeID) const;

	UFUNCTION(BlueprintPure, Category = "Unit Registry|Evolution")
	bool CanEvolve(FName TypeID) const;

	UFUNCTION(BlueprintPure, Category = "Unit Registry")
	int32 GetUnitTier(FName TypeID) const;
};
