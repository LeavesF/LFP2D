#pragma once

#include "CoreMinimal.h"
#include "LFPUnitTypes.generated.h"

// 单位种族枚举
UENUM(BlueprintType)
enum class ELFPUnitRace : uint8
{
	UR_None UMETA(DisplayName = "无"),
	UR_Human UMETA(DisplayName = "人类"),
	UR_WoodSpirit UMETA(DisplayName = "木灵"),
	UR_Featherfolk UMETA(DisplayName = "羽裔"),
	UR_Dragon UMETA(DisplayName = "龙")
};

// 攻击类型枚举
UENUM(BlueprintType)
enum class ELFPAttackType : uint8
{
	AT_Physical UMETA(DisplayName = "物理"),
	AT_Magical UMETA(DisplayName = "魔法")
};

// 轻量单位数据：跨关卡保存用
USTRUCT(BlueprintType)
struct FLFPUnitEntry
{
	GENERATED_BODY()

	// 单位类型 ID（对应 ULFPUnitRegistryDataAsset 中的键）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	FName TypeID = NAME_None;

	// 单位阶级
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit")
	int32 Tier = 1;

	bool IsValid() const { return TypeID != NAME_None; }
};
