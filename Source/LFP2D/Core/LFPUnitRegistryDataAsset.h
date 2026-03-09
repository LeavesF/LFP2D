#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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
};
