#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"

bool ULFPUnitRegistryDataAsset::FindEntry(FName TypeID, FLFPUnitRegistryEntry& OutEntry) const
{
	if (const FLFPUnitRegistryEntry* Found = UnitRegistry.Find(TypeID))
	{
		OutEntry = *Found;
		return true;
	}
	return false;
}

TSubclassOf<ALFPTacticsUnit> ULFPUnitRegistryDataAsset::GetUnitClass(FName TypeID) const
{
	if (const FLFPUnitRegistryEntry* Found = UnitRegistry.Find(TypeID))
	{
		return Found->UnitClass;
	}
	return nullptr;
}
