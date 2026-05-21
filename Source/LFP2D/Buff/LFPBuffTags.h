#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

namespace LFPBuffTags
{
	inline constexpr const TCHAR* EnemyMissDamageBoostBuffIdName = TEXT("Buff.Status.EnemyMissDamageBoost");
	inline constexpr const TCHAR* EnemyMissSpeedBoostBuffIdName = TEXT("Buff.Status.EnemyMissSpeedBoost");

	inline FGameplayTag RequestBuffTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}
}
