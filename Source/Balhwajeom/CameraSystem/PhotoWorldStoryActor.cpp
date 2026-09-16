#include "PhotoWorldStoryActor.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"
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
		PhotoDefinition.StoryCueSound,
		PhotoDefinition.LastCueDurationSeconds);

	// StartStory destroys itself when the authored cues turn out to be unusable.
	return IsValid(StoryActor) ? StoryActor : nullptr;
}

void APhotoWorldStoryActor::StartStory(
	const TArray<FPhotoStoryCue>& InCues,
	const TArray<FText>& LegacyLines,
	const TSoftObjectPtr<USoundBase>& InCueSound,
	const float InLastCueDurationSeconds)
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

	StoryCueSound = InCueSound;
	if (!StoryCueSound.IsNull() && !StoryCueSound.LoadSynchronous())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s failed to load its character Sound Cue; text will continue silently."),
			*GetName());
	}
	const UBalhwajeomInvestigationSettings* Settings =
		GetDefault<UBalhwajeomInvestigationSettings>();
	CurrentCharactersPerSecond = FMath::Max(
		Settings ? Settings->WorldStoryCharactersPerSecond : 20.0f,
		1.0f);
	LastCueDurationSeconds = InLastCueDurationSeconds >= 0.1f
		? InLastCueDurationSeconds
		: FMath::Max(DefaultLastCueDuration, 0.1f);
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

	// Text owns the presentation clock. It starts immediately and never waits for audio.
	CurrentCueIndex = 0;
	ApplyCue(CurrentCueIndex);
	SetStoryWidgetsVisible(true);
	StoryStartTimeSeconds = GetWorld()->GetTimeSeconds();
	ScheduleNextCue();

}

void APhotoWorldStoryActor::PlayCharacterSound(const TCHAR Character)
{
	if (FChar::IsWhitespace(Character) || !StoryAudioComponent)
	{
		return;
	}

	USoundBase* Sound = StoryCueSound.Get();
	if (!Sound)
	{
		return;
	}
	StoryAudioComponent->SetSound(Sound);
	// A Sound Cue can randomize between several short waves. Restarting it for every
	// visible character evaluates that graph again and keeps sound events 1:1 with text.
	StoryAudioComponent->Stop();
	StoryAudioComponent->Play();
}

float APhotoWorldStoryActor::CalculateTypingDuration(
	const int32 TotalCharacterCount,
	const float CharactersPerSecond)
{
	if (TotalCharacterCount <= 1 || CharactersPerSecond <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	return static_cast<float>(TotalCharacterCount - 1) / CharactersPerSecond;
}

void APhotoWorldStoryActor::ApplyCue(int32 CueIndex)
{
	if (!StoryCues.IsValidIndex(CueIndex))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TypewriterTimer);
	CurrentCueFullString = StoryCues[CueIndex].Text.ToString();
	VisibleCharacterCount = 0;
	SetStoryWidgetsText(FText::GetEmpty());
	RevealNextCharacter();
}

void APhotoWorldStoryActor::RevealNextCharacter()
{
	if (VisibleCharacterCount >= CurrentCueFullString.Len())
	{
		GetWorldTimerManager().ClearTimer(TypewriterTimer);
		return;
	}

	const TCHAR RevealedCharacter = CurrentCueFullString[VisibleCharacterCount];
	++VisibleCharacterCount;
	SetStoryWidgetsText(FText::FromString(
		CurrentCueFullString.Left(VisibleCharacterCount)));
	PlayCharacterSound(RevealedCharacter);

	if (VisibleCharacterCount < CurrentCueFullString.Len())
	{
		GetWorldTimerManager().SetTimer(
			TypewriterTimer,
			this,
			&APhotoWorldStoryActor::RevealNextCharacter,
			1.0f / CurrentCharactersPerSecond,
			false);
	}
}

void APhotoWorldStoryActor::ScheduleNextCue()
{
	GetWorldTimerManager().ClearTimer(CueTimer);
	const int32 NextCueIndex = CurrentCueIndex + 1;
	if (!StoryCues.IsValidIndex(NextCueIndex))
	{
		const FPhotoStoryCue& FinalCue = StoryCues[CurrentCueIndex];
		const float TypingDuration = CalculateTypingDuration(
			FinalCue.Text.ToString().Len(), CurrentCharactersPerSecond);
		GetWorldTimerManager().SetTimer(
			CueTimer, this, &APhotoWorldStoryActor::FinishStory,
			TypingDuration + FMath::Max(LastCueDurationSeconds, 0.1f), false);
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

void APhotoWorldStoryActor::StopStory()
{
	if (bFinishing)
	{
		return;
	}

	bFinishing = true;
	GetWorldTimerManager().ClearTimer(CueTimer);
	GetWorldTimerManager().ClearTimer(TypewriterTimer);
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
	GetWorldTimerManager().ClearTimer(TypewriterTimer);

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
	GetWorldTimerManager().ClearTimer(TypewriterTimer);
	GetWorldTimerManager().ClearTimer(FadeTimer);
	GetWorldTimerManager().ClearTimer(ScaleTimer);
	if (StoryAudioComponent)
	{
		StoryAudioComponent->Stop();
	}
	Super::EndPlay(EndPlayReason);
}
