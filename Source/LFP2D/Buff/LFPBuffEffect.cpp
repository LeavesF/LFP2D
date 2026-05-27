#include "LFP2D/Buff/LFPBuffEffect.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPTerrainDataAsset.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

void ULFPBuffEffect::Execute_Implementation(const FLFPBuffEffectContext& Context) const
{
}

FLFPBuffStatModifier ULFPBuffEffect::GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const
{
    return FLFPBuffStatModifier();
}

ULFPBuffEffect_PeriodicDamage::ULFPBuffEffect_PeriodicDamage()
{
    TriggerEvent = ELFPBuffTriggerEvent::OnTurnStart;
}

void ULFPBuffEffect_PeriodicDamage::Execute_Implementation(const FLFPBuffEffectContext& Context) const
{
    ALFPTacticsUnit* TargetUnit = Context.TargetUnit.Get();
    if (!TargetUnit || !TargetUnit->IsAlive() || Damage <= 0)
    {
        return;
    }

    const int32 StackCount = bScaleByStack ? FMath::Max(Context.StackCount, 1) : 1;
    TargetUnit->TakeTrueDamage(Damage * StackCount);
}

ULFPBuffEffect_ModifyStat::ULFPBuffEffect_ModifyStat()
{
    TriggerEvent = ELFPBuffTriggerEvent::PassiveStat;
}

FLFPBuffStatModifier ULFPBuffEffect_ModifyStat::GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const
{
    FLFPBuffStatModifier Result = StatModifier;
    if (bScaleByStack)
    {
        const int32 StackCount = FMath::Max(Context.StackCount, 1);
        Result.AttackDelta *= StackCount;
        Result.PhysicalBlockDelta *= StackCount;
        Result.SpeedDelta *= StackCount;
        Result.OutgoingDamageMultiplier = FMath::Pow(FMath::Max(Result.OutgoingDamageMultiplier, 0.0f), StackCount);
    }

    return Result;
}

ULFPBuffEffect_TerrainStatScaling::ULFPBuffEffect_TerrainStatScaling()
{
    TriggerEvent = ELFPBuffTriggerEvent::PassiveStat;
}

FLFPBuffStatModifier ULFPBuffEffect_TerrainStatScaling::GetStatModifier_Implementation(const FLFPBuffEffectContext& Context) const
{
    ALFPTacticsUnit* TargetUnit = Context.TargetUnit.Get();
    if (!TargetUnit || !TargetUnit->IsAlive())
    {
        return FLFPBuffStatModifier();
    }

    ALFPHexTile* CenterTile = TargetUnit->GetCurrentTile();
    if (!CenterTile)
    {
        return FLFPBuffStatModifier();
    }

    ALFPHexGridManager* GridManager = TargetUnit->GetGridManager();
    if (!GridManager)
    {
        return FLFPBuffStatModifier();
    }

    // BFS 收集距离 <= CheckRange 的所有格子（包括自身）
    TSet<ALFPHexTile*> Visited;
    TArray<ALFPHexTile*> Frontier;
    Visited.Add(CenterTile);
    Frontier.Add(CenterTile);

    for (int32 Step = 0; Step < CheckRange; ++Step)
    {
        TArray<ALFPHexTile*> NextFrontier;
        for (ALFPHexTile* Tile : Frontier)
        {
            for (ALFPHexTile* Neighbor : GridManager->GetNeighbors(Tile->GetCoordinates()))
            {
                if (!Neighbor || Visited.Contains(Neighbor))
                {
                    continue;
                }
                Visited.Add(Neighbor);
                NextFrontier.Add(Neighbor);
            }
        }
        Frontier = MoveTemp(NextFrontier);
    }

    // 统计目标地形数量
    int32 MatchingTileCount = 0;
    for (ALFPHexTile* Tile : Visited)
    {
        const ULFPTerrainDataAsset* TerrainData = Tile->GetTerrainData();
        if (TerrainData && TerrainData->TerrainType == TerrainType)
        {
            ++MatchingTileCount;
        }
    }

    if (MatchingTileCount <= 0)
    {
        return FLFPBuffStatModifier();
    }

    const int32 StackCount = FMath::Max(Context.StackCount, 1);
    const int32 TotalScale = MatchingTileCount * StackCount;

    FLFPBuffStatModifier Result;
    Result.AttackDelta = StatPerStackPerTile.AttackDelta * TotalScale;
    Result.PhysicalBlockDelta = StatPerStackPerTile.PhysicalBlockDelta * TotalScale;
    Result.SpeedDelta = StatPerStackPerTile.SpeedDelta * TotalScale;
    if (!FMath::IsNearlyEqual(StatPerStackPerTile.OutgoingDamageMultiplier, 1.0f))
    {
        Result.OutgoingDamageMultiplier = FMath::Pow(
            FMath::Max(StatPerStackPerTile.OutgoingDamageMultiplier, 0.0f),
            static_cast<float>(TotalScale));
    }
    else
    {
        Result.OutgoingDamageMultiplier = 1.0f;
    }

    return Result;
}
