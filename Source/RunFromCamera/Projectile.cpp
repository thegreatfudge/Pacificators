// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Pacificator.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RunFromCameraCharacter.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	if (!RootComponent)
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSceneComponent"));
	
	if (!CollisionComponent)
	{
		CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
		CollisionComponent->InitSphereRadius(5.f);
		RootComponent = CollisionComponent;
	}

	if (!ProjectileMovementComponent)
	{
		ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
		ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
		ProjectileMovementComponent->InitialSpeed = InitialBulletSpeed;
		ProjectileMovementComponent->MaxSpeed = MaxBulletSpeed;
		ProjectileMovementComponent->bRotationFollowsVelocity = true;
		ProjectileMovementComponent->bShouldBounce = false;
		ProjectileMovementComponent->Bounciness = 1.f;
		ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 200.0f;
	CameraBoom->bUsePawnControlRotation = true;

	BulletCam = CreateDefaultSubobject<UCameraComponent>(TEXT("BulletCam"));
	BulletCam->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	BulletCam->bUsePawnControlRotation = false;

	CollisionComponent->BodyInstance.SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

	InitialLifeSpan = 3.0f;
	bBulletCamActive = false;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::FireInDirection(const FVector& ShootDirection)
{
	ProjectileMovementComponent->Velocity = ShootDirection * ProjectileMovementComponent->InitialSpeed;
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if ( !ProjectileMovementComponent->bShouldBounce )
		Destroy();

	APlayerController* OurPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	ARunFromCameraCharacter* RFC = Cast<ARunFromCameraCharacter>(OurPlayerController->GetPawn());

	if ( bBulletCamActive )
	{
		if (IsValid(RFC))
		{
			bBulletCamActive = false;
			RFC->ResetCameraAfterBulletCam();
			ProjectileMovementComponent->InitialSpeed = 3000.0f;
			ProjectileMovementComponent->MaxSpeed = 3000.0f;
		}
	}

	if (OtherActor->IsA(APacificator::StaticClass()))
	{
		if (IsValid(RFC))
		{
			if (RFC->GetCurrentCamera() == ECameraType::FirstPerson)
			{
				RFC->AddPoints(1);
			}
			else
			{
				RFC->AddPoints(5);
				Destroy();
			}
		}
	}

	if (IsValid(RFC) && OtherActor->IsA(ARunFromCameraCharacter::StaticClass()))
	{
		RFC->Die();
	}
}