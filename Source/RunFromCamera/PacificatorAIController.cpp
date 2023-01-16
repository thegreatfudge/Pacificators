// Fill out your copyright notice in the Description page of Project Settings.

#include "PacificatorAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Pacificator.h"
#include "Kismet/KismetMathLibrary.h"

APacificatorAIController::APacificatorAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APacificatorAIController::Tick(float DeltaTime)
{
	Blackboard = GetBlackboardComponent();

	if ( Blackboard )
	{
		bool bIsEnemyInSight = Blackboard->GetValueAsBool("bEnemyInSight");
		auto ControlledPawn = AController::GetPawn();
		APacificator* PacificatorCamera = Cast<APacificator>(ControlledPawn);
		if ( bIsEnemyInSight && PacificatorCamera )
		{
			auto Target = UKismetMathLibrary::FindLookAtRotation(PacificatorCamera->GetActorLocation(), Blackboard->GetValueAsVector("EnemyPosition"));
			PacificatorCamera->SetActorRotation(FMath::RInterpTo(PacificatorCamera->GetActorRotation(), Target, DeltaTime, CameraTurnRate));
			
			if (PacificatorCamera->Light->GetMaterial(0) != PacificatorCamera->EnemyMaterialInstance)
				PacificatorCamera->Light->SetMaterial(0, PacificatorCamera->EnemyMaterialInstance);
			
			PacificatorCamera->Fire();
		}
		else if ( !bIsEnemyInSight && PacificatorCamera )
		{
			auto Target = UKismetMathLibrary::FindLookAtRotation(PacificatorCamera->GetActorLocation(), Blackboard->GetValueAsVector("RandomPosition"));
			//Get random value for camera rotation for more sudden movements when in "search mode"
			PacificatorCamera->SetActorRotation(FMath::RInterpTo(PacificatorCamera->GetActorRotation(), Target, DeltaTime, FMath::FRandRange(0.5f, CameraTurnRate)));

			if ( PacificatorCamera->Light->GetMaterial(0) != PacificatorCamera->NeutralMaterialInstance )
				PacificatorCamera->Light->SetMaterial(0, PacificatorCamera->NeutralMaterialInstance);
		}
	}

	Super::Tick(DeltaTime);
}

