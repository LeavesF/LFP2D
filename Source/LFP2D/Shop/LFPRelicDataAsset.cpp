#include "LFP2D/Shop/LFPRelicDataAsset.h"

bool ULFPRelicDataAsset::FindRelicDefinition(FName RelicID, FLFPRelicDefinition& OutDefinition) const
{
	if (const FLFPRelicDefinition* Found = RelicMap.Find(RelicID))
	{
		OutDefinition = *Found;
		return true;
	}

	return false;
}
