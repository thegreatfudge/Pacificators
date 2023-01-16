// Fill out your copyright notice in the Description page of Project Settings.


#include "Pacificator.h"
#include "Projectile.h"

// Sets default values
APacificator::APacificator()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	if (!RootComponent)
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PacificatorRootSceneComponent"));

	Light = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);

	Box = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Box"));
	Box->SetupAttachment(RootComponent);

	Lens = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lens"));
	Lens->SetupAttachment(RootComponent);

	MuzzlePoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));
	MuzzlePoint->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UMaterialInstance>Material(TEXT("MaterialInstanceConstant'/Game/Material/Pulse_Material_Green.Pulse_Material_Green'"));
	if (Material.Succeeded())
	{
		NeutralMaterial = Material.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterial>SecondMaterial(TEXT("Material'/Game/Material/Pulse_Material_Red.Pulse_Material_Red'"));
	if (SecondMaterial.Succeeded())
	{
		EnemySpottedMaterial = SecondMaterial.Object;
	}

	bCanShoot = true;
	WeaponFireRate = .75f;
}

// Called when the game starts or when spawned
void APacificator::BeginPlay()
{
	Super::BeginPlay();

	EnemyMaterialInstance = UMaterialInstanceDynamic::Create(EnemySpottedMaterial, Light);
	NeutralMaterialInstance = UMaterialInstanceDynamic::Create(NeutralMaterial, Light);

	//Default camera is non-aggresive
	Light->SetMaterial(0, NeutralMaterialInstance);
}

// Called every frame
void APacificator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APacificator::Fire()
{
	if (ProjectileClass && bCanShoot)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bCanShoot = false;
			GetWorldTimerManager().SetTimer(WeaponCooldown, this, &APacificator::CanShoot, WeaponFireRate, true);
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();

			// Spawn the projectile at the muzzle.
			AProjectile* Projectile = World->SpawnActor<AProjectile>(ProjectileClass, MuzzlePoint->GetComponentLocation(), MuzzlePoint->GetComponentRotation(), SpawnParams);
			
			// Since the projectile is owned by the camera we are looking to find the main character pawn
			Projectile->CollisionComponent->BodyInstance.SetCollisionProfileName(TEXT("Pawn"));
			if (Projectile)
			{
				FVector LaunchDirection = MuzzlePoint->GetComponentRotation().Vector();
				Projectile->FireInDirection(LaunchDirection);
			}
		}
	}
}

// Called to bind functionality to input
void APacificator::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

