#pragma once

#include "CoreMinimal.h"
#include "LFPHireMarketTypes.generated.h"

// 雇佣市场中的单个售卖条目
USTRUCT(BlueprintType)
struct FLFPHireMarketUnitEntry
{
	GENERATED_BODY()

	// 单位类型 ID（对应 UnitRegistry 中的键）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HireMarket")
	FName UnitTypeID = NAME_None;

	// 价格（金币）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HireMarket", meta = (ClampMin = "0"))
	int32 Price = 0;
};

// 单个雇佣市场定义
USTRUCT(BlueprintType)
struct FLFPHireMarketDefinition
{
	GENERATED_BODY()

	// 市场显示名称
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HireMarket")
	FText DisplayName;

	// 可购买的单位列表
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HireMarket")
	TArray<FLFPHireMarketUnitEntry> UnitList;
};
