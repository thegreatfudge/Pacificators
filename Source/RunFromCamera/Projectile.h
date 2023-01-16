// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <Components/SphereComponent.h>
#include <GameFramework/ProjectileMovementComponent.h>
#include "Projectile.generated.h"


UCLASS()
class RUNFROMCAMERA_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void FireInDirection(const FVector& ShootDirection);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	// For random warping around bullet in slow motion using Timelines
	UFUNCTION(BlueprintImplementableEvent)
	void CameraWarp();

	void SetIsBulletCamActive(bool value) { bBulletCamActive = value; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* BulletCam;

	UPROPERTY(VisibleDefaultsOnly, Category = "Projectile | Collision")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile | Movement")
	UProjectileMovementComponent* ProjectileMovementComponent;

private:
	bool bBulletCamActive;

	float MaxBulletSpeed = 3000.0f;
	float InitialBulletSpeed = 3000.0f;
};
