// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"


ULFPSkillBase::ULFPSkillBase()
{
    CurrentCooldown = 0;
    CooldownRounds = 0;
    ActionPointCost = 1;
    bIsDefaultAttack = false;
}

void ULFPSkillBase::Execute(ALFPTacticsUnit* Caster, ALFPHexTile* TargetTile)
{
    // 基类实现为空，由子类重写
}

bool ULFPSkillBase::CanExecute(ALFPTacticsUnit* Caster) const
{
    if (!Caster) return false;

    // 检查冷却
    if (CurrentCooldown > 0) return false;

    // 检查行动点
    if (!Caster->HasEnoughMovePoints(ActionPointCost)) return false;

    return true;
}

FString ULFPSkillBase::GetCooldownStatus() const
{
    if (CurrentCooldown <= 0)
    {
        return FString::Printf(TEXT("Ready"));
    }
    else
    {
        return FString::Printf(TEXT("%d Round"), CurrentCooldown);
    }
}

TArray<ALFPHexTile*> ULFPSkillBase::GetTargetTiles(ALFPTacticsUnit* Caster) const
{
    TArray<ALFPHexTile*> TargetTiles;

    if (!Caster || !Caster->GetCurrentTile()) return TargetTiles;

    ALFPHexGridManager* GridManager = Caster->GetGridManager();
    if (!GridManager) return TargetTiles;

    // 根据技能范围和目标类型获取目标格子
    // 这里需要实现具体的逻辑
    // 例如：获取攻击范围内的所有格子，过滤掉不可用的格子等

    return TargetTiles;
}

void ULFPSkillBase::OnTurnStart()
{
    if (CurrentCooldown > 0)
    {
        CurrentCooldown--;
    }
}