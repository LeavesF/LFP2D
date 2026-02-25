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

bool ULFPSkillBase::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
    if (!Owner) return false;

    // �����ȴ
    //if (CurrentCooldown > 0) return false;

    // ����ж���
    if (!Owner->HasEnoughActionPoints(ActionPointCost)) return false;

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

float ULFPSkillBase::CalculateHatredValue_Implementation(ALFPTacticsUnit* Caster, ALFPTacticsUnit* Target) const
{
    if (!Caster || !Target) return 0.0f;

    // 基础仇恨 = 目标攻击力（威胁越大越优先打）
    float Hatred = (float)Target->GetAttackPower();

    // 距离系数（越近仇恨越高）
    int32 Dist = FLFPHexCoordinates::Distance(
        Caster->GetCurrentCoordinates(),
        Target->GetCurrentCoordinates()
    );
    float DistanceFactor = 1.0f / FMath::Max(Dist, 1);

    return Hatred * DistanceFactor;
}

//void ULFPSkillBase::ShowReleaseRange(bool bShow)
//{
//    if (!ReleaseRangeTiles.IsEmpty())
//    {
//        for (ALFPHexTile* Tile : ReleaseRangeTiles)
//        {
//            if (bShow)
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_Attack);
//            }
//            else
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_Default);
//            }
//        }
//    }
//}
//
//void ULFPSkillBase::ShowEffectRange(bool bShow)
//{
//    if (!ReleaseRangeTiles.IsEmpty())
//    {
//        for (ALFPHexTile* Tile : ReleaseRangeTiles)
//        {
//            if (bShow)
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_SkillEffect);
//            }
//            else
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_Default);
//            }
//        }
//    }
//}
