// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFPTacticsUnit.generated.h"

class ALFPHexTile;

UCLASS()
class LFP2D_API ALFPTacticsUnit : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALFPTacticsUnit();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	// 移动到目标格子
	void MoveToTile(ALFPHexTile* TargetTile);

	// 计算可移动范围
	void CalculateMoveRange();

	UPROPERTY(EditAnywhere)
	int32 MovementRange = 5;

private:
	ALFPHexTile* CurrentTile = nullptr;
};
