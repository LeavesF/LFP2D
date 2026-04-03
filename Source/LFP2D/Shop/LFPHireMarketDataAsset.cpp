#include "LFP2D/Shop/LFPHireMarketDataAsset.h"

bool ULFPHireMarketDataAsset::FindHireMarketDefinition(FName HireMarketID, FLFPHireMarketDefinition& OutDefinition) const
{
	if (const FLFPHireMarketDefinition* Found = HireMarketMap.Find(HireMarketID))
	{
		OutDefinition = *Found;
		return true;
	}

	return false;
}
