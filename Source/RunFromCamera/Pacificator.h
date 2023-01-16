// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/ArrowComponent.h"
#include "Pacificator.generated.h"

UCLASS()
class RUNFROMCAMERA_API APacificator : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APacificator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void Fire();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void CanShoot() { bCanShoot = true; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Light;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Box;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* Lens;

	UPROPERTY(VisibleDefaultsOnly, Category = "Pacificator | Material")
	UMaterialInstanceDynamic* NeutralMaterialInstance;

	UPROPERTY(VisibleDefaultsOnly, Category = "Pacificator | Material")
	UMaterialInstanceDynamic* EnemyMaterialInstance;

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Pacificator | Material")
	UMaterial* EnemySpottedMaterial;

	UPROPERTY(VisibleDefaultsOnly, Category = "Pacificator | Material")
	UMaterialInstance* NeutralMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Pacificator | Shooting")
	UArrowComponent* MuzzlePoint;

	UPROPERTY(EditDefaultsOnly, Category = "Pacificator | Shooting")
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Pacificator | Shooting")
	FTimerHandle WeaponCooldown;

	UPROPERTY(EditDefaultsOnly, Category = "Pacificator | Shooting")
	float WeaponFireRate;

	bool bCanShoot;
};
