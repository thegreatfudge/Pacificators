// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "RunFromCameraCharacter.generated.h"


UENUM(BlueprintType)
enum class ECameraType : uint8
{
	FirstPerson = 0		UMETA(DisplayName = "FirstPerson"),
	ThirdPerson = 1		UMETA(DisplayName = "ThirdPerson"),
	BulletCam = 2		UMETA(DisplayName = "BulletCam")
};


UCLASS(config=Game)
class ARunFromCameraCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCamera;

public:
	ARunFromCameraCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return ThirdPersonCamera; }

	FORCEINLINE class UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	FORCEINLINE ECameraType GetCurrentCamera() const { return CurrentCamera; }

	UFUNCTION(BlueprintCallable)
	int GetPoints() const { return Points; }

	void AddPoints(size_t points) { Points += points; };

	void Die();

	void ResetCameraAfterBulletCam();

protected:
	virtual void Tick(float DeltaTime) override;

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	void ChangePOV();

	void StartSprint();

	void StopSprint();

	void Zoom();

	void CheckSprint(float deltaTime);

	UFUNCTION()
	void Fire();

	void LeftMouseButtonDown();

	void CheckHitForBulletCam(class AProjectile* Projectile, FVector MuzzleLocation, FVector LaunchDirection);

	void CheckBounces();

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Camera")
	ECameraType CurrentCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Camera")
	bool bIsZoomed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Camera")
	float ZoomedCameraFieldOfView;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Camera")
	float DefaultCameraFieldOfView;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	bool bIsSprinting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	float CurrentStaminaLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	float MaxStaminaLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	float StaminaDrainRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	float StaminaRechargeRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement")
	float SprintSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Shooting")
	FVector MuzzleOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Shooting")
	float TimeDilationManipulator;

	UPROPERTY(EditDefaultsOnly, Category = "Character | Shooting")
	TSubclassOf<class AProjectile> ProjectileClass;

	bool bIsLeftMouseButtonDown = false;

private:
	UPROPERTY(VisibleAnywhere, Category = "Character | Points")
	int Points;

};

