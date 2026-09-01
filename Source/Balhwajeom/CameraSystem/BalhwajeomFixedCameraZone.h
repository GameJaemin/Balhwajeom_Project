// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BalhwajeomFixedCameraZone.generated.h"

class ABalhwajeomCameraCharacter;
class UBoxComponent;
class UCameraComponent;
class USceneComponent;

/**
 * A designer-placeable trigger volume with an orbit camera.
 * Mouse X revolves the camera around a designer-positioned center point.
 */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomFixedCameraZone : public AActor
{
	GENERATED_BODY()

public:
	ABalhwajeomFixedCameraZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Called by the controlled character while this is its active camera zone. */
	void AddYawInput(float Value);

	/** Switches the local player's view to this zone camera. */
	void ActivateCamera(ABalhwajeomCameraCharacter* Character);

	/** Switches the local player's view back to the character. */
	void DeactivateCamera(ABalhwajeomCameraCharacter* Character, bool bRestoreCharacterView = true);

	UFUNCTION(BlueprintPure, Category = "Fixed Camera Zone")
	FVector GetPlanarForwardVector() const;

	UFUNCTION(BlueprintPure, Category = "Fixed Camera Zone")
	FVector GetPlanarRightVector() const;

	UFUNCTION(BlueprintPure, Category = "Fixed Camera Zone")
	int32 GetZonePriority() const { return Priority; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Fixed Camera Zone")
	void OnCameraActivated(ABalhwajeomCameraCharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "Fixed Camera Zone")
	void OnCameraDeactivated(ABalhwajeomCameraCharacter* Character);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleZoneEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void ApplyCameraYaw();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fixed Camera Zone")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Resize this component in the level to define the camera-controlled area. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fixed Camera Zone")
	TObjectPtr<UBoxComponent> ZoneVolume;

	/** Move this component to define the center point that the camera orbits. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fixed Camera Zone")
	TObjectPtr<USceneComponent> OrbitPivot;

	/** Move this component relative to OrbitPivot to set orbit radius and height. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fixed Camera Zone")
	TObjectPtr<UCameraComponent> FixedCamera;

	/** Higher-priority overlapping zones win. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone", meta = (ClampMin = "0.0"))
	float BlendTime = 0.35f;

	/** Multiplier applied to the Mouse X axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone", meta = (ClampMin = "0.0"))
	float MouseYawSensitivity = 1.0f;

	/** When enabled, Mouse X can rotate the fixed camera continuously through 360 degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone")
	bool bAllowFullYawRotation = true;

	/** Keeps the camera aimed at OrbitPivot while revolving around it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone")
	bool bKeepCameraFacingCenter = true;

	/** Minimum yaw offset used only when full yaw rotation is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone", meta = (ClampMax = "0.0"))
	float MinYawOffset = -55.0f;

	/** Maximum yaw offset used only when full yaw rotation is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone", meta = (ClampMin = "0.0"))
	float MaxYawOffset = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixed Camera Zone")
	bool bResetYawOnEnter = false;

private:
	FRotator PlacedOrbitRotation = FRotator::ZeroRotator;
	float CurrentYawOffset = 0.0f;
};
