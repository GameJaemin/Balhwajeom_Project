// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomEvidenceTypes.h"
#include "GameFramework/Character.h"
#include "BalhwajeomCameraCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ABalhwajeomFixedCameraZone;
class UBalhwajeomPhotoCameraComponent;
class UBalhwajeomTabletComponent;

/** A keyboard-driven top-down character with hold-to-sprint movement. */
UCLASS()
class BALHWAJEOM_API ABalhwajeomCameraCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABalhwajeomCameraCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Receives Mouse X from ABalhwajeomCameraPlayerController. */
	void ApplyMouseYawInput(float Value);

	/** Called automatically by ABalhwajeomFixedCameraZone overlap events. */
	void RegisterCameraZone(ABalhwajeomFixedCameraZone* Zone);
	void UnregisterCameraZone(ABalhwajeomFixedCameraZone* Zone);

	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsInCameraMode() const;

	UFUNCTION(BlueprintPure, Category = "Evidence")
	TArray<FBalhwajeomEvidenceData> GetCollectedEvidence() const;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartSprinting();
	void StopSprinting();
	void RefreshActiveCameraZone();

	/** Receives Mouse Y from the "LookUp" axis; routes to camera-mode pitch or boom orbit pitch. */
	void HandleLookUp(float Value);

	/**
	 * When true, mouse movement orbits CameraBoom around the character (yaw + pitch) while not in
	 * camera mode and not inside a FixedCameraZone. When false (default), CameraBoom keeps its
	 * fixed world-space angle, matching the original quarter-view behavior.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bAllowCameraOrbit = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	/** First-person viewpoint used while camera mode is active. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;


	/** Normal movement speed in Unreal units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 400.0f;

	/** Movement speed while either Shift key is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float SprintSpeed = 750.0f;

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
};
