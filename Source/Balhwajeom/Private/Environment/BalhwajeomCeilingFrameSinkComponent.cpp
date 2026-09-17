// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/BalhwajeomCeilingFrameSinkComponent.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UBalhwajeomCeilingFrameSinkComponent::UBalhwajeomCeilingFrameSinkComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBalhwajeomCeilingFrameSinkComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bSinkStarted || bSinkFinished)
	{
		SetComponentTickEnabled(false);
		return;
	}

	ElapsedTime += DeltaTime;
	if (ElapsedTime < 0.0f)
	{
		return;
	}

	if (!bMovementStarted)
	{
		BeginSinkMovement();
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		StopEffects();
		bSinkFinished = true;
		SetComponentTickEnabled(false);
		return;
	}

	const float Duration = FMath::Max(KINDA_SMALL_NUMBER, SinkDuration);
	const float LinearAlpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);
	const float AnimationAlpha = SinkCurve
		? FMath::Clamp(SinkCurve->GetFloatValue(LinearAlpha), 0.0f, 1.0f)
		: FMath::InterpEaseInOut(0.0f, 1.0f, LinearAlpha, 2.0f);

	FTransform NewTransform = StartTransform;
	NewTransform.SetLocation(FMath::Lerp(
		StartTransform.GetLocation(), DestinationTransform.GetLocation(), AnimationAlpha));
	if (bApplyDestinationRotation)
	{
		NewTransform.SetRotation(FQuat::Slerp(
			StartTransform.GetRotation(), DestinationTransform.GetRotation(), AnimationAlpha));
	}
	if (bApplyDestinationScale)
	{
		NewTransform.SetScale3D(FMath::Lerp(
			StartTransform.GetScale3D(), DestinationTransform.GetScale3D(), AnimationAlpha));
	}

	Owner->SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (LinearAlpha >= 1.0f)
	{
		FinishSink();
	}
}

void UBalhwajeomCeilingFrameSinkComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	StopEffects();
	Super::EndPlay(EndPlayReason);
}

bool UBalhwajeomCeilingFrameSinkComponent::StartSink()
{
	AActor* Owner = GetOwner();
	if (bSinkStarted || bSinkFinished || !IsValid(Owner) || !IsValid(DestinationActor))
	{
		return false;
	}

	bSinkStarted = true;
	ElapsedTime = -FMath::Max(0.0f, StartDelay);
	SetComponentTickEnabled(true);
	return true;
}

void UBalhwajeomCeilingFrameSinkComponent::BeginSinkMovement()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !IsValid(DestinationActor))
	{
		FinishSink();
		return;
	}

	bMovementStarted = true;
	StartTransform = Owner->GetActorTransform();
	DestinationTransform = DestinationActor->GetActorTransform();
	if (USceneComponent* RootComponent = Owner->GetRootComponent())
	{
		RootComponent->SetMobility(EComponentMobility::Movable);
	}
	if (bDisableCollisionOnStart)
	{
		Owner->SetActorEnableCollision(false);
	}
	SpawnEffects();
}

void UBalhwajeomCeilingFrameSinkComponent::FinishSink()
{
	if (bSinkFinished)
	{
		return;
	}

	if (AActor* Owner = GetOwner(); IsValid(Owner) && bMovementStarted)
	{
		FTransform FinalTransform = StartTransform;
		FinalTransform.SetLocation(DestinationTransform.GetLocation());
		if (bApplyDestinationRotation)
		{
			FinalTransform.SetRotation(DestinationTransform.GetRotation());
		}
		if (bApplyDestinationScale)
		{
			FinalTransform.SetScale3D(DestinationTransform.GetScale3D());
		}
		Owner->SetActorTransform(FinalTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	StopEffects();
	bSinkFinished = true;
	SetComponentTickEnabled(false);
	OnSinkFinished.Broadcast();
}

void UBalhwajeomCeilingFrameSinkComponent::SpawnEffects()
{
	ActiveNiagara = SpawnNiagara();
	ActiveSound = SpawnSound();
}

void UBalhwajeomCeilingFrameSinkComponent::StopEffects()
{
	if (ActiveNiagara.IsValid())
	{
		ActiveNiagara->Deactivate();
	}
	ActiveNiagara.Reset();

	if (bStopSoundOnFinish && ActiveSound.IsValid())
	{
		ActiveSound->Stop();
	}
	ActiveSound.Reset();
}

UNiagaraComponent* UBalhwajeomCeilingFrameSinkComponent::SpawnNiagara() const
{
	const AActor* Owner = GetOwner();
	if (!NiagaraSystem || !IsValid(Owner))
	{
		return nullptr;
	}

	if (NiagaraPlacement == ECeilingFrameEffectPlacement::AttachedToFrame)
	{
		return UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			Owner->GetRootComponent(),
			NAME_None,
			NiagaraOffset,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true);
	}

	const FTransform& EffectTransform =
		NiagaraPlacement == ECeilingFrameEffectPlacement::WorldAtDestination
			? DestinationTransform
			: StartTransform;
	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		NiagaraSystem,
		EffectTransform.GetLocation() + NiagaraOffset,
		EffectTransform.Rotator(),
		FVector::OneVector,
		true);
}

UAudioComponent* UBalhwajeomCeilingFrameSinkComponent::SpawnSound() const
{
	AActor* Owner = GetOwner();
	if (!SinkSound || !IsValid(Owner))
	{
		return nullptr;
	}

	return UGameplayStatics::SpawnSoundAttached(
		SinkSound,
		Owner->GetRootComponent(),
		NAME_None,
		SoundOffset,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false,
		SoundVolumeMultiplier,
		SoundPitchMultiplier);
}

