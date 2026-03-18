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

TArray<FName> ULFPUnitRegistryDataAsset::GetEvolutionTargets(FName TypeID) const
{
	if (const FLFPUnitRegistryEntry* Found = UnitRegistry.Find(TypeID))
	{
		return Found->EvolutionTargets;
	}
	return TArray<FName>();
}

bool ULFPUnitRegistryDataAsset::CanEvolve(FName TypeID) const
{
	if (const FLFPUnitRegistryEntry* Found = UnitRegistry.Find(TypeID))
	{
		return Found->EvolutionTargets.Num() > 0;
	}
	return false;
}

int32 ULFPUnitRegistryDataAsset::GetUnitTier(FName TypeID) const
{
	if (const FLFPUnitRegistryEntry* Found = UnitRegistry.Find(TypeID))
	{
		return Found->Tier;
	}
	return 1;
}
