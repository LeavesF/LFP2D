#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Card/LFPCardTypes.h"

class ULFPCardDataAsset;
class UObject;
struct FLFPUnitRegistryEntry;

struct FLFPCardPreviewAttackDefaults
{
	// 单位通用普攻预览优先使用的数据卡，按攻击类型分别配置。
	TSoftObjectPtr<ULFPCardDataAsset> MeleeAttackCard;
	TSoftObjectPtr<ULFPCardDataAsset> RangedAttackCard;
	TSoftObjectPtr<ULFPCardDataAsset> MagicAttackCard;

	// 兼容旧配置的技能类兜底项。
	TSubclassOf<ULFPSkillBase> MeleeAttackClass;
	TSubclassOf<ULFPSkillBase> RangedAttackClass;
	TSubclassOf<ULFPSkillBase> MagicAttackClass;
};

class LFP2D_API FLFPCardPreviewBuilder
{
public:
	// 根据单位注册表条目构建只读卡牌预览，顺序为通用普攻在前、携带卡在后。
	static void BuildUnitCardsPreview(
		const FLFPUnitRegistryEntry& UnitDefinition,
		const FLFPCardPreviewAttackDefaults& AttackDefaults,
		UObject* Outer,
		int32 MaxCards,
		TArray<FLFPCardInstance>& OutCards);
};
