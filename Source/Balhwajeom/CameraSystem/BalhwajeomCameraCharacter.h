// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomEvidenceTypes.h"
#include "GameFramework/Character.h"
#include "BalhwajeomCameraCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UAnimationAsset;
class ABalhwajeomFixedCameraZone;
class UBalhwajeomPhotoCameraComponent;
class UBalhwajeomTabletComponent;
class UPlayerInteractionComponent;

/** A keyboard-driven top-down character. */
UCLASS()
class BALHWAJEOM_API ABalhwajeomCameraCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABalhwajeomCameraCharacter();
	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Receives Mouse X from ABalhwajeomCameraPlayerController. */
	void ApplyMouseYawInput(float Value);

	/** Called automatically by ABalhwajeomFixedCameraZone overlap events. */
	void RegisterCameraZone(ABalhwajeomFixedCameraZone* Zone);
	void UnregisterCameraZone(ABalhwajeomFixedCameraZone* Zone);

	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsInCameraMode() const;

	/** Restores the correct exploration view after an external interaction camera ends. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void RestoreExplorationView(float BlendTime = 0.35f);

	UFUNCTION(BlueprintPure, Category = "Evidence", meta = (DeprecatedFunction, DeprecationMessage = "A captured-photo list API will be supplied by BalhwajeomInvestigationSubsystem."))
	TArray<FBalhwajeomEvidenceData> GetCollectedEvidence() const;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void RefreshActiveCameraZone();

	/** Receives Mouse Y from the "LookUp" axis; routes to camera-mode pitch or boom orbit pitch. */
	void HandleLookUp(float Value);

	UFUNCTION()
	void HandleInspectionSucceeded(FText InspectionText);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	/** Distance between the character and the third-person camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Third Person", meta = (ClampMin = "0.0"))
	float ThirdPersonArmLength = 400.0f;

	/** Moves the point the camera looks toward. A positive Z places the character lower on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Third Person")
	FVector ThirdPersonTargetOffset = FVector(0.0f, 0.0f, 70.0f);

	/** Offsets the camera at the end of the boom. Positive Y places the character left on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Third Person")
	FVector ThirdPersonSocketOffset = FVector(0.0f, 100.0f, 0.0f);

	/** Initial controller pitch used by the third-person camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Third Person", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float ThirdPersonInitialPitch = -10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Third Person", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float ThirdPersonFieldOfView = 90.0f;

	/** First-person viewpoint used while camera mode is active. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Optional single-node locomotion clips. Leave both unset to keep the existing AnimBP setup. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimationAsset> WalkAnimation;

	/** Horizontal speed at which WalkAnimation replaces IdleAnimation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (ClampMin = "0.0"))
	float WalkAnimationThreshold = 5.0f;


	/** Zones currently containing this character. The highest priority zone is active. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABalhwajeomFixedCameraZone>> OverlappingCameraZones;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ABalhwajeomFixedCameraZone> ActiveCameraZone;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Camera",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBalhwajeomPhotoCameraComponent> PhotoCameraComponent;

	/** Reusable tablet UI/input component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBalhwajeomTabletComponent> TabletComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerInteractionComponent> PlayerInteractionComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> ActiveLocomotionAnimation;
};
