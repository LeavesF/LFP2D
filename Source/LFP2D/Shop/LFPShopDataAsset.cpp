#include "LFP2D/Shop/LFPShopDataAsset.h"

bool ULFPShopDataAsset::FindShopDefinition(FName ShopID, FLFPShopDefinition& OutDefinition) const
{
	if (const FLFPShopDefinition* Found = ShopMap.Find(ShopID))
	{
		OutDefinition = *Found;
		return true;
	}

	return false;
}
