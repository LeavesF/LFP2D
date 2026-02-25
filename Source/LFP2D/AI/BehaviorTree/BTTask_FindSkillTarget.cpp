// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/BehaviorTree/BTTask_FindSkillTarget.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindSkillTarget::UBTTask_FindSkillTarget()
{
    NodeName = "Find Skill Target";
    bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_FindSkillTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return EBTNodeResult::Failed;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard) return EBTNodeResult::Failed;

    // 从黑板中读取要使用的技能
    UObject* SkillObj = Blackboard->GetValueAsObject(SkillKey.SelectedKeyName);
    ULFPSkillBase* Skill = Cast<ULFPSkillBase>(SkillObj);
    if (!Skill) return EBTNodeResult::Failed;

    // 根据技能仇恨值公式找到最优目标
    ALFPTacticsUnit* BestTarget = AIController->FindBestSkillTarget(Skill);
    if (!BestTarget) return EBTNodeResult::Failed;

    Blackboard->SetValueAsObject(TargetUnitKey.SelectedKeyName, BestTarget);
    return EBTNodeResult::Succeeded;
}
