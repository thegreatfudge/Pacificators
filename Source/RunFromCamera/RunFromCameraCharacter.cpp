// Copyright Epic Games, Inc. All Rights Reserved.

#include "RunFromCameraCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

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
}

void ARunFromCameraCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
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
	switch (CurrentCamera)
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
	if (!bIsCrouched)
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
	if (CurrentCamera == ECameraType::FirstPerson)
	{
		if (bIsZoomed)
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
	if (bIsSprinting)
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
