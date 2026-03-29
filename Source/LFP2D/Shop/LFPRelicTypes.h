#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFP2D/Unit/LFPUnitTypes.h"
#include "LFPRelicTypes.generated.h"

// ============== 静态遗物效果（部署时一次性生效） ==============

// 遗物效果类型（第一期只做稳定的战斗数值被动）
UENUM(BlueprintType)
enum class ELFPRelicEffectType : uint8
{
	RET_MaxHealthFlat UMETA(DisplayName = "最大生命"),
	RET_AttackFlat    UMETA(DisplayName = "攻击力"),
	RET_DefenseFlat   UMETA(DisplayName = "防御力"),
	RET_SpeedFlat     UMETA(DisplayName = "速度"),
};

// ============== 战斗遗物规则系统 ==============

// 目标筛选：阵营
UENUM(BlueprintType)
enum class ELFPRelicTargetAffiliation : uint8
{
	RTA_Player  UMETA(DisplayName = "我方"),
	RTA_Enemy   UMETA(DisplayName = "敌方"),
	RTA_Neutral UMETA(DisplayName = "中立"),
};

// 触发时机
UENUM(BlueprintType)
enum class ELFPRelicTriggerType : uint8
{
	RTT_BattleStart      UMETA(DisplayName = "战斗开始"),
	RTT_RoundStart       UMETA(DisplayName = "回合开始"),
	RTT_RoundEnd         UMETA(DisplayName = "回合结束"),
	RTT_UnitTurnStart    UMETA(DisplayName = "单位回合开始"),
	RTT_UnitTurnEnd      UMETA(DisplayName = "单位回合结束"),
	RTT_HealthChanged    UMETA(DisplayName = "血量变化后"),
	RTT_MoveFinished     UMETA(DisplayName = "移动结束后"),
	RTT_AttackAfter      UMETA(DisplayName = "攻击后"),
	RTT_DamagedAfter     UMETA(DisplayName = "受击后"),
	RTT_KillAfter        UMETA(DisplayName = "击杀后"),
};

// 条件类型
UENUM(BlueprintType)
enum class ELFPRelicConditionType : uint8
{
	RCT_None               UMETA(DisplayName = "无条件"),
	RCT_HealthPercentBelow UMETA(DisplayName = "血量比例低于"),
	RCT_HealthPercentAbove UMETA(DisplayName = "血量比例高于"),
	RCT_HealthIsFull       UMETA(DisplayName = "满血"),
	RCT_StatValueBelow     UMETA(DisplayName = "属性值低于"),
	RCT_StatValueAbove     UMETA(DisplayName = "属性值高于"),
	RCT_MovedTilesAtLeast  UMETA(DisplayName = "移动格数至少"),
	RCT_DamageDealtAtLeast UMETA(DisplayName = "造成伤害至少"),
	RCT_HasRelic           UMETA(DisplayName = "拥有遗物"),
	RCT_HasAllRelics       UMETA(DisplayName = "拥有全部遗物"),
};

// 属性类型（条件和效果共用）
UENUM(BlueprintType)
enum class ELFPRelicStatType : uint8
{
	RST_None          UMETA(DisplayName = "无"),
	RST_Attack        UMETA(DisplayName = "攻击力"),
	RST_MaxHealth     UMETA(DisplayName = "最大生命"),
	RST_Speed         UMETA(DisplayName = "速度"),
	RST_PhysicalBlock UMETA(DisplayName = "物理格挡"),
	RST_SpellDefense  UMETA(DisplayName = "法术防御"),
	RST_AttackCount   UMETA(DisplayName = "攻击次数"),
	RST_ActionCount   UMETA(DisplayName = "行动次数"),
};

// 战斗效果类型
UENUM(BlueprintType)
enum class ELFPRelicBattleEffectType : uint8
{
	RBET_HealFlat          UMETA(DisplayName = "回复生命"),
	RBET_DamageFlat        UMETA(DisplayName = "造成伤害"),
	RBET_ModifyStatFlat    UMETA(DisplayName = "属性固定值修改"),
	RBET_ModifyStatPercent UMETA(DisplayName = "属性百分比修改"),
	RBET_AddTag            UMETA(DisplayName = "添加标签"),
	RBET_RemoveTag         UMETA(DisplayName = "移除标签"),
};

// 持续方式
UENUM(BlueprintType)
enum class ELFPRelicDurationType : uint8
{
	RDT_Instant            UMETA(DisplayName = "瞬时"),
	RDT_UntilRoundEnd      UMETA(DisplayName = "持续到回合结束"),
	RDT_UntilUnitTurnEnd   UMETA(DisplayName = "持续到单位回合结束"),
	RDT_WhileConditionTrue UMETA(DisplayName = "条件成立时持续"),
};

// ============== 静态遗物效果结构 ==============

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

// ============== 战斗遗物规则结构 ==============

// 目标筛选器
USTRUCT(BlueprintType)
struct FLFPRelicTargetFilter
{
	GENERATED_BODY()

	// 是否只作用于触发单位自身
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	bool bAffectSelfOnly = true;

	// 生效阵营筛选；为空表示不过滤
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	TArray<ELFPRelicTargetAffiliation> Affiliations;

	// 生效种族筛选；为空表示不过滤
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	TArray<ELFPUnitRace> Races;

	// 必须拥有的标签；为空表示不过滤
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	FGameplayTagContainer RequiredTags;

	// 禁止拥有的标签；为空表示不过滤
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	FGameplayTagContainer BlockedTags;
};

// 单条条件
USTRUCT(BlueprintType)
struct FLFPRelicCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	ELFPRelicConditionType ConditionType = ELFPRelicConditionType::RCT_None;

	// 用于属性类条件
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	ELFPRelicStatType StatType = ELFPRelicStatType::RST_None;

	// 用于整数阈值（如攻击<5、移动5格等）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	int32 IntValue = 0;

	// 用于比例阈值（如血量<40%）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	float FloatValue = 0.f;

	// 用于遗物拥有条件
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	TArray<FName> RelicIDs;
};

// 单条战斗效果
USTRUCT(BlueprintType)
struct FLFPRelicBattleEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	ELFPRelicBattleEffectType EffectType = ELFPRelicBattleEffectType::RBET_HealFlat;

	// 用于属性修改类效果
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	ELFPRelicStatType StatType = ELFPRelicStatType::RST_None;

	// 用于固定值效果
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	int32 IntValue = 0;

	// 用于百分比效果（如 +50% = 0.5）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	float FloatValue = 0.f;

	// 用于 Tag 类效果
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	FGameplayTag Tag;
};

// 单条战斗遗物规则
USTRUCT(BlueprintType)
struct FLFPRelicBattleRule
{
	GENERATED_BODY()

	// 规则 ID（便于调试和运行时状态跟踪）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	FName RuleID = NAME_None;

	// 什么时候检查这条规则
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	ELFPRelicTriggerType TriggerType = ELFPRelicTriggerType::RTT_BattleStart;

	// 对谁生效
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	FLFPRelicTargetFilter TargetFilter;

	// 生效条件（默认全部满足才触发）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	TArray<FLFPRelicCondition> Conditions;

	// 生效效果
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	TArray<FLFPRelicBattleEffect> Effects;

	// 持续方式
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Battle Rule")
	ELFPRelicDurationType DurationType = ELFPRelicDurationType::RDT_Instant;
};

// 遗物组合规则
USTRUCT(BlueprintType)
struct FLFPRelicSynergyRule
{
	GENERATED_BODY()

	// 组合规则 ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Synergy")
	FName SynergyID = NAME_None;

	// 需要同时拥有的遗物
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Synergy")
	TArray<FName> RequiredRelicIDs;

	// 什么时候检查
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Synergy")
	ELFPRelicTriggerType TriggerType = ELFPRelicTriggerType::RTT_BattleStart;

	// 对谁生效
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Synergy")
	FLFPRelicTargetFilter TargetFilter;

	// 生效条件
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Synergy")
	TArray<FLFPRelicCondition> Conditions;

	// 生效效果
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Synergy")
	TArray<FLFPRelicBattleEffect> Effects;

	// 持续方式
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Synergy")
	ELFPRelicDurationType DurationType = ELFPRelicDurationType::RDT_Instant;
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

	// 静态效果：部署/战斗开始时一次性生效
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	TArray<FLFPRelicEffectEntry> Effects;

	// 战斗内动态规则
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
	TArray<FLFPRelicBattleRule> BattleRules;
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
