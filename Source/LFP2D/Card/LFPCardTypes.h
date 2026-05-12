#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "Templates/SubclassOf.h"
#include "LFPCardTypes.generated.h"

class ALFPTacticsUnit;
class UTexture2D;

/* 战斗内卡牌所在区域。v1 只处理玩家牌堆，敌方仍沿用技能 AI。 */
UENUM(BlueprintType)
enum class ELFPCardPile : uint8
{
	DrawPile     UMETA(DisplayName = "DrawPile"),
	Hand         UMETA(DisplayName = "Hand"),
	DiscardPile  UMETA(DisplayName = "DiscardPile"),
	ExhaustPile  UMETA(DisplayName = "ExhaustPile")
};

/* 卡牌静态定义：当前先用技能类承载卡牌效果，后续可以替换为独立 Card DataAsset。 */
USTRUCT(BlueprintType)
struct FLFPCardDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FName CardID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	TSubclassOf<ULFPSkillBase> SkillClass;

	/* 默认出牌成功后进入弃牌堆；特殊消耗牌后续可改成销毁牌堆。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	ELFPCardPile DestinationAfterPlay = ELFPCardPile::DiscardPile;

	bool IsValid() const { return SkillClass != nullptr; }
};

/* 战斗内卡牌实例：一张卡在本场战斗中的唯一运行时状态。 */
USTRUCT(BlueprintType)
struct FLFPCardInstance
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	int32 InstanceID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	FLFPCardDefinition Definition;

	/* 为空表示玩家公共牌库卡；不为空表示该卡来自某个出战单位。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<ALFPTacticsUnit> SourceUnit = nullptr;

	/* 运行时技能实例由卡牌创建，用来复用现有技能范围、目标和结算逻辑。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<ULFPSkillBase> RuntimeSkill = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	ELFPCardPile CurrentPile = ELFPCardPile::DrawPile;

	bool IsValid() const { return InstanceID != INDEX_NONE && RuntimeSkill != nullptr; }
};
