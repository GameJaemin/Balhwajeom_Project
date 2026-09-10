#include "Interaction/BedMemoryActor.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequenceBase.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "CameraSystem/BalhwajeomEvidenceActor.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/CollisionProfile.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Interaction/InspectionComponent.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundMix.h"
#include "Tablet/BalhwajeomTabletComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace BedMemory
{
constexpr double ExitInputGuardSeconds = 0.25;

float RandomInRange(const FVector2D& Range)
{
	const float MinValue = FMath::Min(Range.X, Range.Y);
	const float MaxValue = FMath::Max(Range.X, Range.Y);
	return FMath::FRandRange(MinValue, MaxValue);
}
}

ABedMemoryActor::ABedMemoryActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMesh"));
	BedMesh->SetupAttachment(SceneRoot);
	// The mesh is the solid body of the bed. InteractionCollision remains a
	// query-only trigger, while the assigned bed mesh blocks the player.
	BedMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BedMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BedMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BedMesh->CanCharacterStepUpOn = ECB_No;
	BedMesh->SetStaticMesh(nullptr);

	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(SceneRoot);
	InteractionCollision->SetBoxExtent(FVector(120.0f, 100.0f, 60.0f));
	InteractionCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionCollision->SetGenerateOverlapEvents(true);
	InteractionCollision->CanCharacterStepUpOn = ECB_No;

	InspectionComponent = CreateDefaultSubobject<UInspectionComponent>(TEXT("InspectionComponent"));
	InspectionComponent->CloseDistance = 300.0f;
	InspectionComponent->MiddleDistance = 500.0f;
	InspectionComponent->MaxDisplayDistance = 800.0f;
	InspectionComponent->NearLabel = NSLOCTEXT("BedMemory", "SitPrompt", "[F] 앉기");

	InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(SceneRoot);
	InteractionWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionWidget->SetDrawAtDesiredSize(true);
	InteractionWidget->SetVisibility(false);
	InteractionWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FClassFinder<UUserWidget> LabelWidgetClass(
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel"));
	if (LabelWidgetClass.Succeeded())
	{
		InteractionWidget->SetWidgetClass(LabelWidgetClass.Class);
	}

	PlayerAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PlayerAnchor"));
	PlayerAnchor->SetupAttachment(SceneRoot);
	PlayerAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, 96.0f));

	ExitAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("ExitAnchor"));
	ExitAnchor->SetupAttachment(SceneRoot);
	ExitAnchor->SetRelativeLocation(FVector(0.0f, -180.0f, 96.0f));

	SeatedCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SeatedCamera"));
	SeatedCamera->SetupAttachment(SceneRoot);
	SeatedCamera->SetRelativeLocation(FVector(-220.0f, -220.0f, 190.0f));
	SeatedCamera->SetRelativeRotation(FRotator(-15.0f, 45.0f, 0.0f));
	SeatedCamera->SetAutoActivate(true);

	VoiceOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VoiceOrigin"));
	VoiceOrigin->SetupAttachment(SceneRoot);
	VoiceOrigin->ComponentTags.Add(TEXT("Origin"));

	VoiceDesk = CreateDefaultSubobject<USceneComponent>(TEXT("Voice_Desk"));
	VoiceDesk->SetupAttachment(SceneRoot);
	VoiceDesk->SetRelativeLocation(FVector(200.0f, 0.0f, 100.0f));
	VoiceDesk->ComponentTags.Add(TEXT("Desk"));

	VoiceDoor = CreateDefaultSubobject<USceneComponent>(TEXT("Voice_Door"));
	VoiceDoor->SetupAttachment(SceneRoot);
	VoiceDoor->SetRelativeLocation(FVector(0.0f, -300.0f, 100.0f));
	VoiceDoor->ComponentTags.Add(TEXT("Door"));

	VoiceBed = CreateDefaultSubobject<USceneComponent>(TEXT("Voice_Bed"));
	VoiceBed->SetupAttachment(SceneRoot);
	VoiceBed->SetRelativeLocation(FVector(0.0f, 50.0f, 100.0f));
	VoiceBed->ComponentTags.Add(TEXT("Bed"));

	VoiceHall = CreateDefaultSubobject<USceneComponent>(TEXT("Voice_Hall"));
	VoiceHall->SetupAttachment(SceneRoot);
	VoiceHall->SetRelativeLocation(FVector(0.0f, -500.0f, 100.0f));
	VoiceHall->ComponentTags.Add(TEXT("Hall"));

	VoicePhoto = CreateDefaultSubobject<USceneComponent>(TEXT("Voice_Photo"));
	VoicePhoto->SetupAttachment(SceneRoot);
	VoicePhoto->SetRelativeLocation(FVector(150.0f, 150.0f, 120.0f));
	VoicePhoto->ComponentTags.Add(TEXT("Photo"));

	BGMPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("BGMPlayer"));
	BGMPlayer->SetupAttachment(SceneRoot);
	BGMPlayer->bAutoActivate = false;
	BGMPlayer->bAllowSpatialization = false;

	VoicePlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("VoicePlayer"));
	VoicePlayer->SetupAttachment(VoiceOrigin);
	VoicePlayer->bAutoActivate = false;
	VoicePlayer->bAllowSpatialization = true;

	static ConstructorHelpers::FObjectFinder<UInputAction> InteractAction(
		TEXT("/Game/Balhwajeom/Input/IA_Interact.IA_Interact"));
	if (InteractAction.Succeeded())
	{
		ExitAction = InteractAction.Object;
	}
}

void ABedMemoryActor::BeginPlay()
{
	Super::BeginPlay();

	if (InspectionComponent)
	{
		InspectionComponent->OnPlayerDistanceStateChanged.AddDynamic(
			this, &ABedMemoryActor::HandlePlayerDistanceStateChanged);
	}
	if (VoicePlayer)
	{
		VoicePlayer->OnAudioFinished.AddDynamic(this, &ABedMemoryActor::HandleVoiceFinished);
	}
	if (InteractionCollision)
	{
		InteractionCollision->OnComponentBeginOverlap.AddDynamic(
			this, &ABedMemoryActor::HandleInteractionBeginOverlap);
		InteractionCollision->OnComponentEndOverlap.AddDynamic(
			this, &ABedMemoryActor::HandleInteractionEndOverlap);
	}
}

void ABedMemoryActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (VoiceLoadHandle.IsValid())
	{
		VoiceLoadHandle->CancelHandle();
		VoiceLoadHandle.Reset();
	}
	if (VoicePlayer)
	{
		VoicePlayer->Stop();
	}
	if (BGMPlayer)
	{
		BGMPlayer->Stop();
	}
	if (bSoundMixApplied && BedSoundMix)
	{
		UGameplayStatics::PopSoundMixModifier(this, BedSoundMix);
		bSoundMixApplied = false;
	}
	RestorePlayerState();
	OverlappingPawn = nullptr;
	TeardownRestInput();
	Super::EndPlay(EndPlayReason);
}

bool ABedMemoryActor::CanInteract_Implementation(APawn* InteractingPawn) const
{
	if (State != EBedMemoryState::Idle || !IsValid(InteractingPawn) || !ExitAction ||
		!IsPawnWithinInteractionZone(InteractingPawn))
	{
		return false;
	}

	const UBalhwajeomTabletComponent* Tablet =
		InteractingPawn->FindComponentByClass<UBalhwajeomTabletComponent>();
	if (Tablet && Tablet->IsTabletOpen())
	{
		return false;
	}

	const UBalhwajeomPhotoCameraComponent* PhotoCamera =
		InteractingPawn->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
	return !PhotoCamera ||
		(!PhotoCamera->IsInCameraMode() && !PhotoCamera->IsCameraTransitioning());
}

bool ABedMemoryActor::RequestInteraction_Implementation(APawn* InteractingPawn)
{
	return ToggleRest(InteractingPawn);
}

bool ABedMemoryActor::ToggleRest(APawn* InteractingPawn)
{
	if (State == EBedMemoryState::Idle)
	{
		return BeginRest(InteractingPawn);
	}
	if (IsValid(InteractingPawn) && InteractingPawn == RestingCharacter)
	{
		EndRest();
		return true;
	}
	return false;
}

bool ABedMemoryActor::IsPawnWithinInteractionZone(APawn* Pawn) const
{
	return IsValid(Pawn) && InteractionCollision &&
		(OverlappingPawn == Pawn || InteractionCollision->IsOverlappingActor(Pawn));
}

bool ABedMemoryActor::BeginRest(APawn* InteractingPawn)
{
	if (!CanInteract_Implementation(InteractingPawn))
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(InteractingPawn);
	APlayerController* PlayerController =
		Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Character || !PlayerController || !PlayerController->IsLocalController())
	{
		return false;
	}

	State = EBedMemoryState::AligningPlayer;
	RestingCharacter = Character;
	RestingPlayerController = PlayerController;
	SavedPlayerTransform = Character->GetActorTransform();
	SavedControlRotation = PlayerController->GetControlRotation();
	bHasSavedControlRotation = true;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (Movement)
	{
		SavedMovementMode = static_cast<uint8>(Movement->MovementMode);
		SavedCustomMovementMode = Movement->CustomMovementMode;
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	EarliestExitTimeSeconds = GetWorld()->GetTimeSeconds() + BedMemory::ExitInputGuardSeconds;
	if (!SetupRestInput(PlayerController))
	{
		UE_LOG(LogTemp, Error, TEXT("%s could not install the F-key rest exit input."), *GetName());
		RestorePlayerState();
		State = EBedMemoryState::Idle;
		return false;
	}
	RestInputComponent->bBlockInput = true;

	// Keep the outgoing exploration view fixed while the character is teleported.
	// Otherwise the source camera follows the relocated character during the blend,
	// which makes the transition appear to jump or stutter.
	PlayerController->SetViewTargetWithBlend(
		this,
		CameraBlendInDuration,
		VTBlend_Cubic,
		0.0f,
		true);

	if (PlayerAnchor)
	{
		const FRotator AnchorFacingRotation(
			0.0f, PlayerAnchor->GetComponentRotation().Yaw, 0.0f);
		FHitResult IgnoredHit;
		Character->SetActorLocationAndRotation(
			PlayerAnchor->GetComponentLocation(),
			AnchorFacingRotation,
			false,
			&IgnoredHit,
			ETeleportType::TeleportPhysics);
		PlayerController->SetControlRotation(AnchorFacingRotation);
	}

	if (AHUD* HUD = PlayerController->GetHUD())
	{
		bSavedHUDVisible = HUD->bShowHUD;
		HUD->bShowHUD = false;
	}
	SetInspectionLabelSuppressed(true);
	SetWorldEvidenceLabelsSuppressed(true);

	// Rotate during the camera transition. Sitting animation, BGM, and memory
	// voices remain disconnected until their implementation step.
	BeginPlayerTurn();
	return State != EBedMemoryState::Idle;
}

void ABedMemoryActor::BeginPlayerTurn()
{
	if (State != EBedMemoryState::AligningPlayer || !RestingCharacter || !PlayerAnchor)
	{
		RestorePlayerState();
		State = EBedMemoryState::Idle;
		return;
	}

	PlayerTurnStartRotation = RestingCharacter->GetActorRotation();
	PlayerTurnTargetRotation = PlayerTurnStartRotation;
	PlayerTurnTargetRotation.Yaw += 180.0f;
	PlayerTurnStartedAtSeconds = GetWorld()->GetTimeSeconds();
	if (PlayerRotationDuration <= UE_KINDA_SMALL_NUMBER)
	{
		FinishPlayerTurn();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PlayerTurnTimer, this, &ABedMemoryActor::UpdatePlayerTurn, 0.02f, true);
	UpdatePlayerTurn();
}

void ABedMemoryActor::UpdatePlayerTurn()
{
	if (State != EBedMemoryState::AligningPlayer || !RestingCharacter)
	{
		GetWorldTimerManager().ClearTimer(PlayerTurnTimer);
		return;
	}

	const float Alpha = FMath::Clamp(
		static_cast<float>(GetWorld()->GetTimeSeconds() - PlayerTurnStartedAtSeconds) /
			PlayerRotationDuration,
		0.0f,
		1.0f);
	const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	FRotator NewRotation = PlayerTurnStartRotation;
	NewRotation.Yaw = PlayerTurnStartRotation.Yaw + 180.0f * EasedAlpha;
	RestingCharacter->SetActorRotation(NewRotation);
	if (RestingPlayerController)
	{
		RestingPlayerController->SetControlRotation(NewRotation);
	}
	if (Alpha >= 1.0f)
	{
		FinishPlayerTurn();
	}
}

void ABedMemoryActor::FinishPlayerTurn()
{
	if (State != EBedMemoryState::AligningPlayer || !RestingCharacter)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PlayerTurnTimer);
	RestingCharacter->SetActorRotation(PlayerTurnTargetRotation);
	if (RestingPlayerController)
	{
		RestingPlayerController->SetControlRotation(PlayerTurnTargetRotation);
	}
	BeginEntering();
}

void ABedMemoryActor::BeginEntering()
{
	if (State != EBedMemoryState::AligningPlayer || !RestingCharacter)
	{
		return;
	}

	SavePlayerAnimationState();
	State = EBedMemoryState::Entering;
	const float EnterDuration = PlayBedAnimation(SitAnimation, false, 1.0f, 0.0f);
	if (EnterDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			TransitionTimer,
			this,
			&ABedMemoryActor::FinishEntering,
			EnterDuration,
			false);
	}
	else
	{
		FinishEntering();
	}
}

void ABedMemoryActor::FinishEntering()
{
	if (State != EBedMemoryState::Entering || !RestingCharacter)
	{
		return;
	}

	State = EBedMemoryState::Seated;
	if (SeatedIdleMontage)
	{
		PlayBedAnimation(SeatedIdleMontage, true, 1.0f, 0.0f);
	}
	BeginPreparingAudio();
}

void ABedMemoryActor::BeginPreparingAudio()
{
	State = EBedMemoryState::PreparingAudio;
	BuildVoiceCandidates();

	TArray<FSoftObjectPath> Paths;
	for (const FBedMemoryVoiceCandidate& Candidate : VoiceCandidates)
	{
		const FSoftObjectPath Path = Candidate.StoryVoice.ToSoftObjectPath();
		if (Path.IsValid())
		{
			Paths.AddUnique(Path);
		}
	}

	if (Paths.IsEmpty())
	{
		BeginListening();
		return;
	}

	VoiceLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths,
		FStreamableDelegate::CreateUObject(this, &ABedMemoryActor::HandleVoiceAssetsLoaded));
	if (!VoiceLoadHandle.IsValid())
	{
		HandleVoiceAssetsLoaded();
	}
}

void ABedMemoryActor::BuildVoiceCandidates()
{
	VoiceCandidates.Reset();
	ShuffleBag.Reset();

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UBalhwajeomInvestigationSubsystem* Investigation =
		GameInstance ? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>() : nullptr;
	if (!Investigation)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s could not resolve BalhwajeomInvestigationSubsystem."), *GetName());
		return;
	}

	TArray<FCapturedPhotoRecord> CapturedPhotos;
	Investigation->GetCapturedPhotos(CapturedPhotos);
	for (const FCapturedPhotoRecord& Record : CapturedPhotos)
	{
		FPhotoDefinition Definition;
		if (!Investigation->GetPhotoDefinition(Record.PhotoID, Definition))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s could not resolve captured PhotoID '%s'."),
				*GetName(), *Record.PhotoID.ToString());
			continue;
		}
		if (Definition.StoryVoice.IsNull())
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: captured PhotoID '%s' has no StoryVoice."),
				*GetName(), *Record.PhotoID.ToString());
			continue;
		}

		FBedMemoryVoiceCandidate Candidate;
		Candidate.PhotoID = Record.PhotoID;
		Candidate.StoryVoice = Definition.StoryVoice;
		Candidate.EmitterID = ResolveEmitterID(Record.PhotoID);
		VoiceCandidates.Add(MoveTemp(Candidate));
	}
}

void ABedMemoryActor::HandleVoiceAssetsLoaded()
{
	if (State != EBedMemoryState::PreparingAudio)
	{
		return;
	}

	VoiceCandidates.RemoveAll([this](const FBedMemoryVoiceCandidate& Candidate)
	{
		if (Candidate.StoryVoice.Get())
		{
			return false;
		}
		UE_LOG(LogTemp, Warning, TEXT("%s failed to load StoryVoice for PhotoID '%s'."),
			*GetName(), *Candidate.PhotoID.ToString());
		return true;
	});
	BeginListening();
}

void ABedMemoryActor::BeginListening()
{
	if (State != EBedMemoryState::PreparingAudio)
	{
		return;
	}
	State = EBedMemoryState::Listening;
	if (!VoiceCandidates.IsEmpty())
	{
		ScheduleNextVoice(true);
	}
}

void ABedMemoryActor::ScheduleNextVoice(bool bInitialDelay)
{
	if (State != EBedMemoryState::Listening || VoiceCandidates.IsEmpty())
	{
		return;
	}

	float Delay = 0.0f;
	if (bInitialDelay)
	{
		Delay = BedMemory::RandomInRange(InitialDelayRange);
	}
	else if (VoiceCandidates.Num() == 1 || FMath::FRand() < LongGapChance)
	{
		Delay = BedMemory::RandomInRange(LongGapRange);
	}
	else
	{
		Delay = BedMemory::RandomInRange(NormalGapRange);
	}

	if (Delay <= UE_KINDA_SMALL_NUMBER)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this, &ABedMemoryActor::PlayNextVoice);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			VoiceTimer, this, &ABedMemoryActor::PlayNextVoice, Delay, false);
	}
}

void ABedMemoryActor::RefillShuffleBag()
{
	ShuffleBag.Reset();
	for (int32 Index = 0; Index < VoiceCandidates.Num(); ++Index)
	{
		ShuffleBag.Add(Index);
	}
	for (int32 Index = ShuffleBag.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		ShuffleBag.Swap(Index, SwapIndex);
	}
	if (ShuffleBag.Num() > 1 &&
		VoiceCandidates[ShuffleBag.Last()].PhotoID == LastPlayedPhotoID)
	{
		ShuffleBag.Swap(0, ShuffleBag.Num() - 1);
	}
}

void ABedMemoryActor::PlayNextVoice()
{
	if (State != EBedMemoryState::Listening || !VoicePlayer || VoiceCandidates.IsEmpty())
	{
		return;
	}
	if (ShuffleBag.IsEmpty())
	{
		RefillShuffleBag();
	}
	if (ShuffleBag.IsEmpty())
	{
		return;
	}

	const int32 CandidateIndex = ShuffleBag.Pop(EAllowShrinking::No);
	if (!VoiceCandidates.IsValidIndex(CandidateIndex))
	{
		ScheduleNextVoice(false);
		return;
	}

	const FBedMemoryVoiceCandidate& Candidate = VoiceCandidates[CandidateIndex];
	USoundBase* Sound = Candidate.StoryVoice.Get();
	if (!Sound)
	{
		ScheduleNextVoice(false);
		return;
	}

	USceneComponent* Emitter = ResolveEmitter(Candidate.EmitterID);
	VoicePlayer->SetWorldLocation(Emitter ? Emitter->GetComponentLocation() : GetActorLocation());
	VoicePlayer->SetSound(Sound);
	LastPlayedPhotoID = Candidate.PhotoID;
	VoicePlayer->Play();
}

void ABedMemoryActor::HandleVoiceFinished()
{
	if (State == EBedMemoryState::Listening)
	{
		ScheduleNextVoice(false);
	}
}

void ABedMemoryActor::HandleToggleInput()
{
	APawn* Pawn = State == EBedMemoryState::Idle
		? OverlappingPawn.Get()
		: RestingCharacter.Get();
	if (GetWorld() && GetWorld()->GetTimeSeconds() >= EarliestExitTimeSeconds)
	{
		ToggleRest(Pawn);
	}
}

void ABedMemoryActor::HandleInteractionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	APlayerController* PlayerController =
		Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!Pawn || !PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	OverlappingPawn = Pawn;
	if (SetupRestInput(PlayerController) && State == EBedMemoryState::Idle)
	{
		RestInputComponent->bBlockInput = false;
	}
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}

void ABedMemoryActor::HandleInteractionEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor != OverlappingPawn)
	{
		return;
	}
	if (State == EBedMemoryState::Idle)
	{
		OverlappingPawn = nullptr;
		ApplyInspectionDistanceState(LastInspectionDistanceState);
		TeardownRestInput();
	}
}

void ABedMemoryActor::EndRest()
{
	if (State == EBedMemoryState::Idle ||
		State == EBedMemoryState::Exiting ||
		State == EBedMemoryState::Restoring)
	{
		return;
	}
	if (State == EBedMemoryState::AligningPlayer)
	{
		GetWorldTimerManager().ClearTimer(PlayerTurnTimer);
		State = EBedMemoryState::Restoring;
		if (ABalhwajeomCameraCharacter* CameraCharacter =
			Cast<ABalhwajeomCameraCharacter>(RestingCharacter))
		{
			CameraCharacter->RestoreExplorationView(CameraBlendInDuration);
		}
		else if (RestingPlayerController && RestingCharacter)
		{
			RestingPlayerController->SetViewTargetWithBlend(
				RestingCharacter, CameraBlendInDuration, VTBlend_Cubic);
		}
		RestorePlayerState();
		State = EBedMemoryState::Idle;
		ApplyInspectionDistanceState(LastInspectionDistanceState);
		return;
	}

	State = EBedMemoryState::Exiting;
	GetWorldTimerManager().ClearTimer(PlayerTurnTimer);
	GetWorldTimerManager().ClearTimer(TransitionTimer);
	GetWorldTimerManager().ClearTimer(VoiceTimer);
	if (VoiceLoadHandle.IsValid())
	{
		VoiceLoadHandle->CancelHandle();
		VoiceLoadHandle.Reset();
	}
	if (VoicePlayer)
	{
		VoicePlayer->FadeOut(0.2f, 0.0f);
	}
	if (BGMPlayer)
	{
		BGMPlayer->FadeOut(BGMFadeOutDuration, 0.0f);
	}
	if (bSoundMixApplied && BedSoundMix)
	{
		UGameplayStatics::PopSoundMixModifier(this, BedSoundMix);
		bSoundMixApplied = false;
	}

	float ExitDuration = 0.0f;
	if (RestingCharacter && SitAnimation)
	{
		float ReverseStartPosition = SitAnimation->GetPlayLength();
		if (USkeletalMeshComponent* Mesh = RestingCharacter->GetMesh())
		{
			if (UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance())
			{
				if (SingleNode->GetCurrentAsset() == SitAnimation)
				{
					ReverseStartPosition = SingleNode->GetCurrentTime();
				}
			}
		}
		ExitDuration = PlayBedAnimation(
			SitAnimation, false, -1.0f, ReverseStartPosition);
	}
	if (ExitDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			TransitionTimer, this, &ABedMemoryActor::FinishExiting, ExitDuration, false);
	}
	else
	{
		FinishExiting();
	}
}

void ABedMemoryActor::FinishExiting()
{
	if (State != EBedMemoryState::Exiting)
	{
		return;
	}
	State = EBedMemoryState::Restoring;

	if (RestingCharacter && ExitAnchor)
	{
		const FRotator ExitFacingRotation(
			0.0f, ExitAnchor->GetComponentRotation().Yaw, 0.0f);
		FHitResult IgnoredHit;
		RestingCharacter->SetActorLocationAndRotation(
			ExitAnchor->GetComponentLocation(),
			ExitFacingRotation,
			false,
			&IgnoredHit,
			ETeleportType::TeleportPhysics);

		// A completed exit uses ExitAnchor as the authoritative facing direction.
		// Preserve the player's previous camera pitch while replacing its yaw so
		// controller-driven characters do not turn back toward the bed next frame.
		SavedControlRotation.Yaw = ExitFacingRotation.Yaw;
		if (RestingPlayerController)
		{
			FRotator ExitControlRotation = SavedControlRotation;
			ExitControlRotation.Yaw = ExitFacingRotation.Yaw;
			RestingPlayerController->SetControlRotation(ExitControlRotation);
		}
	}

	if (ABalhwajeomCameraCharacter* CameraCharacter = Cast<ABalhwajeomCameraCharacter>(RestingCharacter))
	{
		CameraCharacter->RestoreExplorationView(CameraBlendInDuration);
	}
	else if (RestingPlayerController && RestingCharacter)
	{
		RestingPlayerController->SetViewTargetWithBlend(
			RestingCharacter, CameraBlendInDuration, VTBlend_Cubic);
	}

	RestorePlayerState();
	State = EBedMemoryState::Idle;
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}

void ABedMemoryActor::SavePlayerAnimationState()
{
	if (bHasSavedAnimationState || !RestingCharacter || !RestingCharacter->GetMesh())
	{
		return;
	}

	USkeletalMeshComponent* Mesh = RestingCharacter->GetMesh();
	SavedAnimationMode = static_cast<uint8>(Mesh->GetAnimationMode());
	SavedAnimInstanceClass = Mesh->GetAnimClass();
	SavedAnimationAsset = nullptr;
	SavedAnimationPosition = 0.0f;
	SavedAnimationPlayRate = 1.0f;
	bSavedAnimationLooping = false;
	bSavedAnimationPlaying = false;
	if (UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance())
	{
		SavedAnimationAsset = SingleNode->GetCurrentAsset();
		SavedAnimationPosition = SingleNode->GetCurrentTime();
		SavedAnimationPlayRate = SingleNode->GetPlayRate();
		bSavedAnimationLooping = SingleNode->IsLooping();
		bSavedAnimationPlaying = SingleNode->IsPlaying();
	}
	bHasSavedAnimationState = true;
}

float ABedMemoryActor::PlayBedAnimation(
	UAnimationAsset* Animation,
	bool bLooping,
	float PlayRate,
	float StartPosition)
{
	if (!Animation || !RestingCharacter || !RestingCharacter->GetMesh() ||
		FMath::IsNearlyZero(PlayRate))
	{
		return 0.0f;
	}

	USkeletalMeshComponent* Mesh = RestingCharacter->GetMesh();
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->SetAnimation(Animation);
	UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance();
	if (!SingleNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s could not create a single-node animation instance."), *GetName());
		return 0.0f;
	}

	const float Length = Animation->GetPlayLength();
	const float ClampedStart = FMath::Clamp(StartPosition, 0.0f, Length);
	SingleNode->SetLooping(bLooping);
	SingleNode->SetPlayRate(PlayRate);
	SingleNode->SetPosition(ClampedStart, false);
	SingleNode->SetPlaying(true);

	if (bLooping)
	{
		return 0.0f;
	}
	const float DistanceToTravel = PlayRate < 0.0f
		? ClampedStart
		: Length - ClampedStart;
	return DistanceToTravel / FMath::Abs(PlayRate);
}

void ABedMemoryActor::RestorePlayerAnimationState()
{
	if (!bHasSavedAnimationState || !RestingCharacter || !RestingCharacter->GetMesh())
	{
		return;
	}

	USkeletalMeshComponent* Mesh = RestingCharacter->GetMesh();
	const EAnimationMode::Type AnimationMode =
		static_cast<EAnimationMode::Type>(SavedAnimationMode);
	Mesh->SetAnimationMode(AnimationMode);
	if (AnimationMode == EAnimationMode::AnimationBlueprint)
	{
		Mesh->SetAnimInstanceClass(SavedAnimInstanceClass);
	}
	else if (AnimationMode == EAnimationMode::AnimationSingleNode && SavedAnimationAsset)
	{
		Mesh->SetAnimation(SavedAnimationAsset);
		if (UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance())
		{
			SingleNode->SetLooping(bSavedAnimationLooping);
			SingleNode->SetPlayRate(SavedAnimationPlayRate);
			SingleNode->SetPosition(SavedAnimationPosition, false);
			SingleNode->SetPlaying(bSavedAnimationPlaying);
		}
	}

	SavedAnimationAsset = nullptr;
	SavedAnimInstanceClass = nullptr;
	bHasSavedAnimationState = false;
}

void ABedMemoryActor::RestorePlayerState()
{
	RestorePlayerAnimationState();
	if (RestingPlayerController)
	{
		RestingPlayerController->SetIgnoreMoveInput(false);
		RestingPlayerController->SetIgnoreLookInput(false);
		if (bHasSavedControlRotation)
		{
			RestingPlayerController->SetControlRotation(SavedControlRotation);
		}
		if (AHUD* HUD = RestingPlayerController->GetHUD())
		{
			HUD->bShowHUD = bSavedHUDVisible;
		}
	}
	if (RestingCharacter)
	{
		if (UCharacterMovementComponent* Movement = RestingCharacter->GetCharacterMovement())
		{
			Movement->SetMovementMode(
				static_cast<EMovementMode>(SavedMovementMode), SavedCustomMovementMode);
		}
	}

	SetWorldEvidenceLabelsSuppressed(false);
	bInspectionLabelSuppressed = false;

	const bool bStillOverlapping = RestingCharacter &&
		InteractionCollision && InteractionCollision->IsOverlappingActor(RestingCharacter);
	if (bStillOverlapping && RestInputComponent)
	{
		OverlappingPawn = RestingCharacter;
		RestInputComponent->bBlockInput = false;
	}
	else
	{
		OverlappingPawn = nullptr;
		TeardownRestInput();
	}
	VoiceCandidates.Reset();
	ShuffleBag.Reset();
	RestingCharacter = nullptr;
	RestingPlayerController = nullptr;
	bHasSavedControlRotation = false;
}

bool ABedMemoryActor::SetupRestInput(APlayerController* PlayerController)
{
	if (RestInputComponent)
	{
		return true;
	}
	if (!PlayerController || !ExitAction)
	{
		return false;
	}

	RestInputComponent = NewObject<UEnhancedInputComponent>(
		PlayerController,
		MakeUniqueObjectName(PlayerController, UEnhancedInputComponent::StaticClass(), TEXT("BedMemoryInput")));
	RestInputComponent->Priority = RestInputPriority;
	RestInputComponent->bBlockInput = true;
	RestInputComponent->RegisterComponent();
	FEnhancedInputActionEventBinding& ToggleBinding = RestInputComponent->BindAction(
		ExitAction, ETriggerEvent::Started, this, &ABedMemoryActor::HandleToggleInput);
	ToggleBinding.SetShouldConsume(true);
	PlayerController->PushInputComponent(RestInputComponent);
	return true;
}

void ABedMemoryActor::TeardownRestInput()
{
	if (!RestInputComponent)
	{
		return;
	}
	if (RestingPlayerController)
	{
		RestingPlayerController->PopInputComponent(RestInputComponent);
	}
	RestInputComponent->DestroyComponent();
	RestInputComponent = nullptr;
}

FName ABedMemoryActor::ResolveEmitterID(FName PhotoID) const
{
	if (const FName* Found = PhotoEmitterMap.Find(PhotoID))
	{
		return *Found;
	}
	return TEXT("Origin");
}

USceneComponent* ABedMemoryActor::ResolveEmitter(FName EmitterID) const
{
	TInlineComponentArray<USceneComponent*> Components(this);
	for (USceneComponent* Component : Components)
	{
		if (Component && Component->ComponentHasTag(EmitterID))
		{
			return Component;
		}
	}
	return VoiceOrigin;
}

void ABedMemoryActor::HandlePlayerDistanceStateChanged(EPlayerInspectionDistanceState NewState)
{
	LastInspectionDistanceState = NewState;
	ApplyInspectionDistanceState(NewState);
}

void ABedMemoryActor::SetInspectionLabelSuppressed(bool bSuppressed)
{
	bInspectionLabelSuppressed = bSuppressed;
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}

void ABedMemoryActor::ApplyInspectionDistanceState(EPlayerInspectionDistanceState DistanceState)
{
	const bool bShow = !bInspectionLabelSuppressed &&
		State == EBedMemoryState::Idle &&
		IsValid(OverlappingPawn) &&
		IsPawnWithinInteractionZone(OverlappingPawn);
	SetInspectionLabel(InspectionComponent ? InspectionComponent->NearLabel : FText::GetEmpty(), bShow);
}

void ABedMemoryActor::SetInspectionLabel(const FText& LabelText, bool bVisible)
{
	if (!InteractionWidget)
	{
		return;
	}
	const bool bShouldShow = bVisible && !LabelText.IsEmptyOrWhitespace();
	InteractionWidget->SetVisibility(bShouldShow);
	if (!bShouldShow)
	{
		return;
	}

	InteractionWidget->InitWidget();
	UUserWidget* Widget = InteractionWidget->GetUserWidgetObject();
	UFunction* Function = Widget ? Widget->FindFunction(TEXT("SetLabelText")) : nullptr;
	if (!Function)
	{
		return;
	}
	struct FSetLabelTextParameters
	{
		FText NewText;
	};
	FSetLabelTextParameters Parameters{LabelText};
	Widget->ProcessEvent(Function, &Parameters);
}

void ABedMemoryActor::SetWorldEvidenceLabelsSuppressed(bool bSuppressed) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ABalhwajeomEvidenceActor> It(World); It; ++It)
	{
		It->SetInspectionLabelSuppressed(bSuppressed);
	}
}
