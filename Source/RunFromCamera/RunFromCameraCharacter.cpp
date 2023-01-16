// Copyright Epic Games, Inc. All Rights Reserved.

#include "RunFromCameraCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Projectile.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Pacificator.h"

//////////////////////////////////////////////////////////////////////////
// ARunFromCameraCharacter

ARunFromCameraCharacter::ARunFromCameraCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	ThirdPersonCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetMesh(), "head");
	FirstPersonCamera->SetRelativeRotation({ -90.f, 0.f, 90.f});
	FirstPersonCamera->SetRelativeLocation({ 0.f, 10.f, 0.f});
	FirstPersonCamera->bUsePawnControlRotation = true;

	CurrentCamera = ECameraType::ThirdPerson;
	bIsZoomed = false;
	DefaultCameraFieldOfView = 90.f;
	ZoomedCameraFieldOfView = 30.f;

	bIsSprinting = false;

	CurrentStaminaLevel = 1.f;
	MaxStaminaLevel = 1.f;;
	StaminaDrainRate = .1f;
	StaminaRechargeRate = .15f;

	SprintSpeed = 1100.f;
	WalkSpeed = 500.f;

	Points = 0;
	TimeDilationManipulator = .10f;
}

//////////////////////////////////////////////////////////////////////////
// Input

void ARunFromCameraCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("RightMouseButton", IE_Pressed, this, &ARunFromCameraCharacter::ChangePOV);
	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ARunFromCameraCharacter::Zoom);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARunFromCameraCharacter::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARunFromCameraCharacter::StopSprint);

	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ARunFromCameraCharacter::Fire);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ARunFromCameraCharacter::LeftMouseButtonDown);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ARunFromCameraCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ARunFromCameraCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ARunFromCameraCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ARunFromCameraCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ARunFromCameraCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ARunFromCameraCharacter::TouchStopped);
}

void ARunFromCameraCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ARunFromCameraCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ARunFromCameraCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ARunFromCameraCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ARunFromCameraCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckSprint(DeltaTime);

	if ( bIsLeftMouseButtonDown && CurrentCamera == ECameraType::ThirdPerson )
	{
		CheckBounces();
	}
}

void ARunFromCameraCharacter::MoveForward(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ARunFromCameraCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}


void ARunFromCameraCharacter::ChangePOV()
{
	switch ( CurrentCamera )
	{
		case ECameraType::ThirdPerson:
			CurrentCamera = ECameraType::FirstPerson;
			ThirdPersonCamera->SetActive(false);
			FirstPersonCamera->SetActive(true);
			bUseControllerRotationYaw = true;
			break;

		case ECameraType::FirstPerson:
			CurrentCamera = ECameraType::ThirdPerson;
			ThirdPersonCamera->SetActive(true);
			FirstPersonCamera->SetActive(false);
			bUseControllerRotationYaw = false;
			bIsZoomed = false;
			break;

		default:
			UE_LOG(LogTemp, Error, TEXT("Current camera type is not viable!"));

	}
}

void ARunFromCameraCharacter::StartSprint()
{
	if ( !bIsCrouched )
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void ARunFromCameraCharacter::StopSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ARunFromCameraCharacter::Zoom()
{
	if ( CurrentCamera == ECameraType::FirstPerson )
	{
		if ( bIsZoomed )
		{
			bIsZoomed = false;
			FirstPersonCamera->SetFieldOfView(DefaultCameraFieldOfView);
		}
		else
		{
			bIsZoomed = true;
			FirstPersonCamera->SetFieldOfView(ZoomedCameraFieldOfView);
		}
	}
}

void ARunFromCameraCharacter::CheckSprint(float deltaTime)
{
	if ( bIsSprinting )
	{
		if (CurrentStaminaLevel > 0.f)
		{
			CurrentStaminaLevel = FMath::FInterpConstantTo(CurrentStaminaLevel, 0.f, deltaTime, StaminaDrainRate);
		}
		//We've hit 0 stamina. Stop sprinting.
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
			bIsSprinting = false;
		}
	}
	//Recharge stamina
	else
	{
		if (CurrentStaminaLevel < MaxStaminaLevel)
		{
			CurrentStaminaLevel = FMath::FInterpConstantTo(CurrentStaminaLevel, MaxStaminaLevel, deltaTime, StaminaRechargeRate);
		}
	}
}


void ARunFromCameraCharacter::Fire()
{
	if ( ProjectileClass )
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		GetActorEyesViewPoint(CameraLocation, CameraRotation);

		MuzzleOffset.Set(100.0f, 0.0f, 0.0f);

		FVector MuzzleLocation = CameraLocation + FTransform(CameraRotation).TransformVector(MuzzleOffset);
		FRotator MuzzleRotation = CameraRotation;

		UWorld* World = GetWorld();
		if ( World )
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();

			// Spawn the projectile at the muzzle.
			AProjectile* Projectile = World->SpawnActor<AProjectile>(ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);

			if ( CurrentCamera == ECameraType::ThirdPerson )
				Projectile->ProjectileMovementComponent->bShouldBounce = true;

			if ( Projectile )
			{
				// Set the projectile's initial trajectory.
				FVector LaunchDirection = MuzzleRotation.Vector();
				if ( CurrentCamera == ECameraType::FirstPerson )
					CheckHitForBulletCam(Projectile, MuzzleLocation, LaunchDirection);
				Projectile->FireInDirection(LaunchDirection);
			}
		}
	}
	bIsLeftMouseButtonDown = false;
}

void ARunFromCameraCharacter::LeftMouseButtonDown()
{
	bIsLeftMouseButtonDown = true;
}


void ARunFromCameraCharacter::CheckHitForBulletCam(AProjectile* Projectile, FVector MuzzleLocation, FVector LaunchDirection)
{
	FPredictProjectilePathParams params;
	params.StartLocation = MuzzleLocation;
	params.LaunchVelocity = LaunchDirection * 3000.f;
	params.bTraceWithChannel = true;
	params.ProjectileRadius = 5.f;
	params.TraceChannel = ECC_Pawn;
	params.bTraceWithCollision = true;
	params.ActorsToIgnore.Add(this);
	params.OverrideGravityZ = 1.f;

	FPredictProjectilePathResult result;

	UGameplayStatics::PredictProjectilePath(GetWorld(), params, result);

	if (result.HitResult.bBlockingHit)
	{
		APlayerController* OurPlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (result.HitResult.GetActor()->IsA(APacificator::StaticClass()))
		{
			auto camera = Projectile->BulletCam;
			if (OurPlayerController)
			{
				Projectile->SetIsBulletCamActive(true);
				Projectile->CameraWarp();

				//Set bigger speeds for bullet to be faster than turret ones to avoid getting killed while bullet cam is active
				Projectile->ProjectileMovementComponent->InitialSpeed = 4500.f;
				Projectile->ProjectileMovementComponent->MaxSpeed = 6000.f;
				CurrentCamera = ECameraType::BulletCam;
				this->DisableInput(OurPlayerController);

				UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TimeDilationManipulator);
				OurPlayerController->SetViewTargetWithBlend(Projectile, TimeDilationManipulator);
			}
			bUseControllerRotationYaw = false;
		}
	}
}


void ARunFromCameraCharacter::CheckBounces()
{
	TArray<FVector> Bounces;

	// Initial data setup
	FVector CameraLocation;
	FRotator CameraRotation;
	GetActorEyesViewPoint(CameraLocation, CameraRotation);
	MuzzleOffset.Set(0.f, 0.0f, 0.0f);

	FVector MuzzleLocation = CameraLocation + FTransform(CameraRotation).TransformVector(MuzzleOffset);
	FRotator MuzzleRotation = CameraRotation;
	FVector LaunchDirection = MuzzleRotation.Vector();

	FPredictProjectilePathParams params;
	params.StartLocation = MuzzleLocation;
	params.LaunchVelocity = LaunchDirection * 3000.f;
	params.bTraceWithChannel = true;
	params.ProjectileRadius = 5.f;
	params.TraceChannel = ECC_Pawn;
	params.bTraceWithCollision = true;
	params.ActorsToIgnore.Add(this);
	params.OverrideGravityZ = 1.f;

	FPredictProjectilePathResult result;
	UGameplayStatics::PredictProjectilePath(GetWorld(), params, result);

	//Step 1 of bouncing
	if (result.HitResult.bBlockingHit)
	{
		// Move the impact point along the normal of Impact point - so that the middle of the point is not on the wall but 
		// moved further from the wall to be more in-line with the collision of an object
		auto Direction = result.HitResult.ImpactNormal;
		Direction *= (params.ProjectileRadius * 2);
		params.StartLocation = result.HitResult.ImpactPoint + Direction;

		Bounces.Add(MuzzleLocation);
		Bounces.Add(params.StartLocation);

		params.ActorsToIgnore.Add(result.HitResult.GetActor());

		params.LaunchVelocity = UKismetMathLibrary::MirrorVectorByNormal(params.LaunchVelocity, Direction); 
		params.LaunchVelocity *= 3000.f;

		UGameplayStatics::PredictProjectilePath(GetWorld(), params, result);
		if (result.HitResult.bBlockingHit)
		{
			Direction = result.HitResult.ImpactNormal;
			Direction *= (params.ProjectileRadius * 2);
			params.StartLocation = result.HitResult.ImpactPoint + Direction;

			Bounces.Add(params.StartLocation);


			params.ActorsToIgnore.Empty();
			params.ActorsToIgnore.Add(result.HitResult.GetActor());

			params.LaunchVelocity = UKismetMathLibrary::MirrorVectorByNormal(params.LaunchVelocity, Direction);
			UGameplayStatics::PredictProjectilePath(GetWorld(), params, result);

			//Two bounces found. Draw the path with debug lines/spheres.
			if (result.HitResult.bBlockingHit)
			{
				DrawDebugLine(GetWorld(), Bounces[0], Bounces[1], FColor::Green);
				DrawDebugSphere(GetWorld(), Bounces[1], 10.f, 16, FColor::Red);
				DrawDebugLine(GetWorld(), Bounces[1], Bounces[2], FColor::Blue);
				DrawDebugSphere(GetWorld(), Bounces[2], 10.f, 16, FColor::Red);
				DrawDebugLine(GetWorld(), params.StartLocation, result.HitResult.ImpactPoint, FColor::Red);
				DrawDebugSphere(GetWorld(), result.HitResult.ImpactPoint, 15.f, 16, FColor::Red);

			}
		}
	}
}

void ARunFromCameraCharacter::Die()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void ARunFromCameraCharacter::ResetCameraAfterBulletCam()
{
	APlayerController* OurPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	this->EnableInput(OurPlayerController);
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	OurPlayerController->SetViewTarget(this);
	CurrentCamera = ECameraType::FirstPerson;
	ThirdPersonCamera->SetActive(false);
	FirstPersonCamera->SetActive(true);
	bUseControllerRotationYaw = true;
}


