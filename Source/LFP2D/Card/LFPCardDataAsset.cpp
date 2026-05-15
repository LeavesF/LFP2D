#include "LFP2D/Card/LFPCardDataAsset.h"

FLFPCardDefinition ULFPCardDataAsset::BuildCardDefinition() const
{
	FLFPCardDefinition Definition;
	Definition.CardID = CardID.IsNone() ? GetFName() : CardID;
	Definition.DisplayName = DisplayName;
	Definition.Description = Description;
	Definition.Icon = Icon;
	Definition.ActionPointCost = ActionPointCost;
	Definition.SkillClass = SkillClass;
	Definition.DestinationAfterPlay = DestinationAfterPlay;
	Definition.CardCategory = CardCategory;
	Definition.RequiredTag = RequiredTag;

	return Definition;
}
