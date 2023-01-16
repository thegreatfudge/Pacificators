// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include <BehaviorTree/BehaviorTreeTypes.h>
#include "PacificatorAIController.generated.h"

/**
 * 
 */
UCLASS()
class RUNFROMCAMERA_API APacificatorAIController : public AAIController
{
	GENERATED_BODY()
	
	virtual void Tick(float DeltaTime) override;

public:

	APacificatorAIController();

	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector BlackboardKey;

private:
	float CameraTurnRate = 3.0f;
};
