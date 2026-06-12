#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

namespace LFPBuffTags
{
	inline constexpr const TCHAR* EnemyMissDamageBoostBuffIdName = TEXT("Buff.Status.EnemyMissDamageBoost");
	inline constexpr const TCHAR* EnemyMissSpeedBoostBuffIdName = TEXT("Buff.Status.EnemyMissSpeedBoost");
	inline constexpr const TCHAR* RootedBuffIdName = TEXT("Buff.Status.Rooted");
	inline constexpr const TCHAR* PoisonBuffIdName = TEXT("Buff.Status.Poison");
	inline constexpr const TCHAR* CounterVineBuffIdName = TEXT("Buff.Passive.CounterVine");

	inline FGameplayTag RequestBuffTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}
}
