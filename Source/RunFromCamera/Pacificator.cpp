// Fill out your copyright notice in the Description page of Project Settings.


#include "Pacificator.h"

// Sets default values
APacificator::APacificator()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APacificator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APacificator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APacificator::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

