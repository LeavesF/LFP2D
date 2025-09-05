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
    // ����ʵ��Ϊ�գ���������д
}

bool ULFPSkillBase::CanExecute(ALFPTacticsUnit* Caster) const
{
    if (!Caster) return false;

    // �����ȴ
    if (CurrentCooldown > 0) return false;

    // ����ж���
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

    // ���ݼ��ܷ�Χ��Ŀ�����ͻ�ȡĿ�����
    // ������Ҫʵ�־�����߼�
    // ���磺��ȡ������Χ�ڵ����и��ӣ����˵������õĸ��ӵ�

    return TargetTiles;
}

void ULFPSkillBase::OnTurnStart()
{
    if (CurrentCooldown > 0)
    {
        CurrentCooldown--;
    }
}