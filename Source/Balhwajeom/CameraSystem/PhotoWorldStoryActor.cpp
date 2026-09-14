#include "PhotoWorldStoryActor.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "PhotoWorldStoryWidget.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

APhotoWorldStoryActor::APhotoWorldStoryActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StoryWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StoryWidget"));
	StoryWidgetComponent->SetupAttachment(SceneRoot);

	BackStoryWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("BackStoryWidget"));
	BackStoryWidgetComponent->SetupAttachment(SceneRoot);
	BackStoryWidgetComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	const auto ConfigureStoryPanel = [](UWidgetComponent* Component)
	{
		Component->SetWidgetSpace(EWidgetSpace::World);
		Component->SetDrawAtDesiredSize(true);
		// Opposing front faces avoid the dim, mirrored back face of a single two-sided panel.
		Component->SetTwoSided(false);
		Component->SetBlendMode(EWidgetBlendMode::Transparent);
		Component->SetBackgroundColor(FLinearColor::Transparent);
		Component->SetTintColorAndOpacity(FLinearColor::White);
		Component->SetPivot(FVector2D(0.5f, 0.5f));
		Component->SetRelativeScale3D(FVector(0.1f));
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	};
	ConfigureStoryPanel(StoryWidgetComponent);
	ConfigureStoryPanel(BackStoryWidgetComponent);

	static ConstructorHelpers::FClassFinder<UPhotoWorldStoryWidget> DefaultStoryWidget(
		TEXT("/Game/Balhwajeom/UI/PhotoStory/WBP_PhotoWorldStory"));
	StoryWidgetClass = UPhotoWorldStoryWidget::StaticClass();
	if (DefaultStoryWidget.Succeeded())
	{
		StoryWidgetClass = DefaultStoryWidget.Class;
	}
	StoryWidgetComponent->SetWidgetClass(StoryWidgetClass);
	StoryWidgetComponent->SetVisibility(false);
	BackStoryWidgetComponent->SetWidgetClass(StoryWidgetClass);
	BackStoryWidgetComponent->SetVisibility(false);

	StoryAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("StoryAudio"));
	StoryAudioComponent->SetupAttachment(SceneRoot);
	StoryAudioComponent->bAutoActivate = false;
	StoryAudioComponent->bAllowSpatialization = false;
	StoryAudioComponent->OnAudioFinished.AddDynamic(this, &APhotoWorldStoryActor::HandleAudioFinished);
}

void APhotoWorldStoryActor::InitializeStoryWidgets()
{
	for (UWidgetComponent* Component : { StoryWidgetComponent.Get(), BackStoryWidgetComponent.Get() })
	{
		if (!Component)
		{
			continue;
		}
		if (StoryWidgetClass)
		{
			Component->SetWidgetClass(StoryWidgetClass);
		}
		Component->InitWidget();
	}
}

void APhotoWorldStoryActor::SetStoryWidgetsVisible(bool bVisible)
{
	if (StoryWidgetComponent)
	{
		StoryWidgetComponent->SetVisibility(bVisible);
	}
	if (BackStoryWidgetComponent)
	{
		BackStoryWidgetComponent->SetVisibility(bVisible);
	}
}

void APhotoWorldStoryActor::SetStoryWidgetsText(const FText& Text)
{
	for (UWidgetComponent* Component : { StoryWidgetComponent.Get(), BackStoryWidgetComponent.Get() })
	{
		if (UPhotoWorldStoryWidget* Widget = Component
			? Cast<UPhotoWorldStoryWidget>(Component->GetUserWidgetObject()) : nullptr)
		{
			Widget->SetStoryText(Text);
			Widget->SetRenderOpacity(1.0f);
		}
	}
}

void APhotoWorldStoryActor::SetStoryWidgetsOpacity(float Opacity)
{
	for (UWidgetComponent* Component : { StoryWidgetComponent.Get(), BackStoryWidgetComponent.Get() })
	{
		if (UUserWidget* Widget = Component ? Component->GetUserWidgetObject() : nullptr)
		{
			Widget->SetRenderOpacity(Opacity);
		}
	}
}

void APhotoWorldStoryActor::SetStoryWidgetsFontSize(int32 FontSize)
{
	for (UWidgetComponent* Component : { StoryWidgetComponent.Get(), BackStoryWidgetComponent.Get() })
	{
		if (UPhotoWorldStoryWidget* Widget = Component
			? Cast<UPhotoWorldStoryWidget>(Component->GetUserWidgetObject()) : nullptr)
		{
			Widget->SetStoryFontSize(FontSize);
		}
	}
}

APhotoWorldStoryActor* APhotoWorldStoryActor::SpawnAndStart(
	UWorld* World,
	TSubclassOf<APhotoWorldStoryActor> StoryClass,
	const FTransform& SpawnTransform,
	const FPhotoDefinition& PhotoDefinition,
	AActor* Owner)
{
	if (!World ||
		(PhotoDefinition.WorldStoryCues.IsEmpty() && PhotoDefinition.WorldStoryLines.IsEmpty()))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Owner;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APhotoWorldStoryActor* StoryActor = World->SpawnActor<APhotoWorldStoryActor>(
		StoryClass ? *StoryClass : APhotoWorldStoryActor::StaticClass(),
		SpawnTransform,
		SpawnParameters);
	if (!StoryActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn photo world story for '%s'."),
			*PhotoDefinition.PhotoID.ToString());
		return nullptr;
	}

	StoryActor->StartStory(
		PhotoDefinition.WorldStoryCues,
		PhotoDefinition.WorldStoryLines,
		PhotoDefinition.StoryVoice);

	// StartStory destroys itself when the authored cues turn out to be unusable.
	return IsValid(StoryActor) ? StoryActor : nullptr;
}

void APhotoWorldStoryActor::StartStory(
	const TArray<FPhotoStoryCue>& InCues,
	const TArray<FText>& LegacyLines,
	const TSoftObjectPtr<USoundBase>& InVoice)
{
	StoryCues = InCues;
	StoryCues.StableSort([](const FPhotoStoryCue& A, const FPhotoStoryCue& B)
	{
		return A.StartTimeSeconds < B.StartTimeSeconds;
	});

	if (StoryCues.IsEmpty())
	{
		for (int32 Index = 0; Index < LegacyLines.Num(); ++Index)
		{
			if (!LegacyLines[Index].IsEmpty())
			{
				FPhotoStoryCue& Cue = StoryCues.AddDefaulted_GetRef();
				Cue.Text = LegacyLines[Index];
				Cue.StartTimeSeconds = Index * LegacySecondsPerLine;
			}
		}
	}

	StoryVoice = InVoice;
	if (StoryCues.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s cannot start: story cues are missing."), *GetName());
		Destroy();
		return;
	}

	InitializeStoryWidgets();
	if (bThirdPersonScaleRequested)
	{
		TransitionToThirdPersonScale();
	}
	if (StoryVoice.IsNull())
	{
		bPlayingWithoutVoice = true;
		CurrentCueIndex = 0;
		ApplyCue(CurrentCueIndex);
		SetStoryWidgetsVisible(true);
		StoryStartTimeSeconds = GetWorld()->GetTimeSeconds();
		ScheduleNextCue();
		return;
	}

	if (StoryVoice.Get())
	{
		PlayLoadedVoice();
		return;
	}

	VoiceLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		StoryVoice.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &APhotoWorldStoryActor::HandleVoiceLoaded));
	if (!VoiceLoadHandle.IsValid())
	{
		HandleVoiceLoaded();
	}
}

void APhotoWorldStoryActor::HandleVoiceLoaded()
{
	VoiceLoadHandle.Reset();
	if (!IsActorBeingDestroyed() && StoryVoice.Get())
	{
		PlayLoadedVoice();
	}
	else if (!IsActorBeingDestroyed())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s failed to load StoryVoice."), *GetName());
		Destroy();
	}
}

void APhotoWorldStoryActor::PlayLoadedVoice()
{
	USoundBase* Voice = StoryVoice.Get();
	if (!Voice || !StoryAudioComponent || StoryCues.IsEmpty())
	{
		Destroy();
		return;
	}

	CurrentCueIndex = 0;
	bPlayingWithoutVoice = false;
	ApplyCue(CurrentCueIndex);
	SetStoryWidgetsVisible(true);
	StoryStartTimeSeconds = GetWorld()->GetTimeSeconds();
	StoryAudioComponent->SetSound(Voice);
	StoryAudioComponent->Play();
	ScheduleNextCue();
}

void APhotoWorldStoryActor::ApplyCue(int32 CueIndex)
{
	if (!StoryCues.IsValidIndex(CueIndex))
	{
		return;
	}

	SetStoryWidgetsText(StoryCues[CueIndex].Text);
}

void APhotoWorldStoryActor::ScheduleNextCue()
{
	GetWorldTimerManager().ClearTimer(CueTimer);
	const int32 NextCueIndex = CurrentCueIndex + 1;
	if (!StoryCues.IsValidIndex(NextCueIndex))
	{
		if (bPlayingWithoutVoice)
		{
			GetWorldTimerManager().SetTimer(
				CueTimer, this, &APhotoWorldStoryActor::FinishStory,
				FMath::Max(NoVoiceLastCueDuration, 0.1f), false);
		}
		return;
	}

	const double Elapsed = GetWorld()->GetTimeSeconds() - StoryStartTimeSeconds;
	const float Delay = FMath::Max(StoryCues[NextCueIndex].StartTimeSeconds - Elapsed, 0.001);
	GetWorldTimerManager().SetTimer(CueTimer, this, &APhotoWorldStoryActor::HandleCueTimer, Delay, false);
}

void APhotoWorldStoryActor::HandleCueTimer()
{
	const double Elapsed = GetWorld()->GetTimeSeconds() - StoryStartTimeSeconds;
	while (StoryCues.IsValidIndex(CurrentCueIndex + 1) &&
		StoryCues[CurrentCueIndex + 1].StartTimeSeconds <= Elapsed + KINDA_SMALL_NUMBER)
	{
		++CurrentCueIndex;
		ApplyCue(CurrentCueIndex);
	}
	ScheduleNextCue();
}

void APhotoWorldStoryActor::HandleAudioFinished()
{
	FinishStory();
}

void APhotoWorldStoryActor::StopStory()
{
	if (bFinishing)
	{
		return;
	}

	bFinishing = true;
	GetWorldTimerManager().ClearTimer(CueTimer);
	if (StoryAudioComponent && StoryAudioComponent->IsPlaying())
	{
		StoryAudioComponent->Stop();
	}
	bFinishing = false;
	FinishStory();
}

void APhotoWorldStoryActor::TransitionToThirdPersonScale()
{
	bThirdPersonScaleRequested = true;
	if (!StoryWidgetComponent)
	{
		return;
	}

	InitializeStoryWidgets();
	UPhotoWorldStoryWidget* Widget =
		Cast<UPhotoWorldStoryWidget>(StoryWidgetComponent->GetUserWidgetObject());
	if (!Widget)
	{
		return;
	}

	const float Multiplier = FMath::Max(Widget->GetThirdPersonFontSizeMultiplier(), 1.0f);
	FontSizeTransitionStart = FMath::Max(Widget->GetStoryFontSize(), 1);
	FontSizeTransitionTarget = FMath::Max(
		FMath::RoundToInt(FontSizeTransitionStart * Multiplier), FontSizeTransitionStart);
	ScaleTransitionDuration = FMath::Max(Widget->GetThirdPersonScaleTransitionDuration(), 0.0f);
	GetWorldTimerManager().ClearTimer(ScaleTimer);

	if (ScaleTransitionDuration <= KINDA_SMALL_NUMBER)
	{
		SetStoryWidgetsFontSize(FontSizeTransitionTarget);
		return;
	}

	ScaleStartTimeSeconds = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(
		ScaleTimer, this, &APhotoWorldStoryActor::UpdateScaleTransition, 0.02f, true);
}

void APhotoWorldStoryActor::UpdateScaleTransition()
{
	const double Elapsed = GetWorld()->GetTimeSeconds() - ScaleStartTimeSeconds;
	const float Alpha = FMath::Clamp(Elapsed / ScaleTransitionDuration, 0.0, 1.0);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	SetStoryWidgetsFontSize(FMath::RoundToInt(
		FMath::Lerp(static_cast<float>(FontSizeTransitionStart),
			static_cast<float>(FontSizeTransitionTarget), SmoothAlpha)));

	if (Alpha >= 1.0f)
	{
		GetWorldTimerManager().ClearTimer(ScaleTimer);
	}
}

void APhotoWorldStoryActor::FinishStory()
{
	if (bFinishing)
	{
		return;
	}
	bFinishing = true;
	GetWorldTimerManager().ClearTimer(CueTimer);

	if (FadeOutDuration <= KINDA_SMALL_NUMBER)
	{
		Destroy();
		return;
	}

	FadeStartTimeSeconds = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(
		FadeTimer, this, &APhotoWorldStoryActor::UpdateFadeOut, 0.02f, true);
}

void APhotoWorldStoryActor::UpdateFadeOut()
{
	const double Elapsed = GetWorld()->GetTimeSeconds() - FadeStartTimeSeconds;
	const float Opacity = 1.0f - FMath::Clamp(Elapsed / FadeOutDuration, 0.0, 1.0);
	SetStoryWidgetsOpacity(Opacity);
	if (Opacity <= 0.0f)
	{
		Destroy();
	}
}

void APhotoWorldStoryActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bFinishing = true;
	GetWorldTimerManager().ClearTimer(CueTimer);
	GetWorldTimerManager().ClearTimer(FadeTimer);
	GetWorldTimerManager().ClearTimer(ScaleTimer);
	if (VoiceLoadHandle.IsValid())
	{
		VoiceLoadHandle->CancelHandle();
		VoiceLoadHandle.Reset();
	}
	if (StoryAudioComponent)
	{
		StoryAudioComponent->Stop();
	}
	Super::EndPlay(EndPlayReason);
}
