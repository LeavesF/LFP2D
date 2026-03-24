#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "LFPRelicTypes.generated.h"

// 遗物效果类型（第一期只做稳定的战斗数值被动）
UENUM(BlueprintType)
enum class ELFPRelicEffectType : uint8
{
	RET_MaxHealthFlat UMETA(DisplayName = "最大生命"),
	RET_AttackFlat    UMETA(DisplayName = "攻击力"),
	RET_DefenseFlat   UMETA(DisplayName = "防御力"),
	RET_SpeedFlat     UMETA(DisplayName = "速度"),
};

// 单条遗物效果
USTRUCT(BlueprintType)
struct FLFPRelicEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	ELFPRelicEffectType EffectType = ELFPRelicEffectType::RET_MaxHealthFlat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	int32 Value = 0;
};

// 遗物定义
USTRUCT(BlueprintType)
struct FLFPRelicDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	TArray<FLFPRelicEffectEntry> Effects;
};

// 商店中的单个售卖条目
USTRUCT(BlueprintType)
struct FLFPShopRelicEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FName RelicID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 Price = 0;
};

// 单个商店定义
USTRUCT(BlueprintType)
struct FLFPShopDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TArray<FLFPShopRelicEntry> RelicList;
};
