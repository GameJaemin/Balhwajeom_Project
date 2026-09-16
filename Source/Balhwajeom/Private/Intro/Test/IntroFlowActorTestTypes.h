#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Intro/BalhwajeomIntroFlowActor.h"
#include "UI/BalhwajeomCinematicVideoWidget.h"
#include "UI/BalhwajeomScreenFadeWidget.h"

/** Drives ABalhwajeomIntroFlowActor's private flow from automation tests. */
struct FIntroFlowActorTestAccessor
{
	static void SetResetPhotosOnStart(
		ABalhwajeomIntroFlowActor* IntroFlow,
		bool bEnabled)
	{
		IntroFlow->bResetInvestigationPhotosOnStart = bEnabled;
	}

	static void PrepareTitleScreen(ABalhwajeomIntroFlowActor* IntroFlow)
	{
		IntroFlow->State = EBalhwajeomIntroState::Title;
		IntroFlow->ScreenFadeWidget =
			NewObject<UBalhwajeomScreenFadeWidget>(IntroFlow);
	}

	/** Puts the actor in the state it reaches once a movie is on screen and playing. */
	static void PrepareCinematic(
		ABalhwajeomIntroFlowActor* IntroFlow,
		EBalhwajeomIntroState CinematicState)
	{
		IntroFlow->State = CinematicState;
		IntroFlow->ScreenFadeWidget =
			NewObject<UBalhwajeomScreenFadeWidget>(IntroFlow);
		IntroFlow->CinematicVideoWidget =
			NewObject<UBalhwajeomCinematicVideoWidget>(IntroFlow);
	}

	static void RequestStart(ABalhwajeomIntroFlowActor* IntroFlow)
	{
		IntroFlow->HandleStartRequested();
	}

	static void RequestSkip(ABalhwajeomIntroFlowActor* IntroFlow)
	{
		IntroFlow->HandleSkipRequested();
	}

	static void FinishFadeToBlack(ABalhwajeomIntroFlowActor* IntroFlow)
	{
		IntroFlow->HandleFadeToBlackFinished();
	}

	static UBalhwajeomCinematicVideoWidget* GetCinematicVideoWidget(
		const ABalhwajeomIntroFlowActor* IntroFlow)
	{
		return IntroFlow->CinematicVideoWidget;
	}

	static EBalhwajeomIntroState GetState(
		const ABalhwajeomIntroFlowActor* IntroFlow)
	{
		return IntroFlow->State;
	}
};

#endif
