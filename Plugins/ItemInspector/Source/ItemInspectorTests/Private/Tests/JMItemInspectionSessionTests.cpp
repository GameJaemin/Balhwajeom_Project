#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Core/ItemInspectorSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "ItemInspection/JMItemInspectionSubsystem.h"
#include "Tests/AutomationEditorCommon.h"

struct FJMInspectionTestAccessor
{
	static void SetState(UJMItemInspectionSubsystem* S, EJMItemInspectionState State) { S->State = State; }
	static auto Resolve(UJMItemInspectionSubsystem* S, const FJMItemInspectionRequest& R) { return S->ResolveTransitionSettings(R); }
	static void Block(UJMItemInspectionSubsystem* S, APlayerController* PC) { S->ApplyPlayerInputBlock(PC, true); }
	static void Restore(UJMItemInspectionSubsystem* S) { S->RestorePlayerInputBlock(); }
	static void OwnSource(UJMItemInspectionSubsystem* S, AActor* A)
	{
		// Each assertion models a fresh inspection session. Do not let the
		// previous source-ownership assertion leak into the next one.
		S->HiddenSourceActor = A;
		S->bSourceActorWasHidden = A->IsHidden();
		S->bShouldHideSourceActor = true;
		S->bDidHideSourceActor = false;
		S->HideSourceActorIfNeeded();
	}
	static void RestoreSource(UJMItemInspectionSubsystem* S) { S->RestoreSourceActor(); }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMInspectionSessionTest, "JM.ItemInspector.Session.RestorationAndPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMInspectionSessionTest::RunTest(const FString& Parameters)
{
	ULocalPlayer* LP = NewObject<ULocalPlayer>(GEngine);
	UJMItemInspectionSubsystem* S = NewObject<UJMItemInspectionSubsystem>(LP);
	UJMItemInspectionData* Data = NewObject<UJMItemInspectionData>();
	FJMItemInspectionRequest R; R.InspectionData = Data;
	TestEqual(TEXT("Project default precedence"), FJMInspectionTestAccessor::Resolve(S, R).EnterDuration,
		UItemInspectorSettings::Get()->DefaultTransitionSettings.EnterDuration);
	Data->bOverrideTransitionSettings = true; Data->TransitionSettings.EnterDuration = 0.71f;
	TestEqual(TEXT("Data overrides project"), FJMInspectionTestAccessor::Resolve(S, R).EnterDuration, 0.71f);
	R.bOverrideTransitionSettings = true; R.TransitionSettings.EnterDuration = 0.93f;
	TestEqual(TEXT("Request overrides data"), FJMInspectionTestAccessor::Resolve(S, R).EnterDuration, 0.93f);
	FJMInspectionTestAccessor::SetState(S, EJMItemInspectionState::TransitioningOut);
	TestFalse(TEXT("Open rejected while closing"), S->OpenInspectionFromRequest(R));
	TestEqual(TEXT("Rejection preserves closing state"), S->GetInspectionState(), EJMItemInspectionState::TransitioningOut);
	FJMInspectionTestAccessor::SetState(S, EJMItemInspectionState::Inspecting);
	TestFalse(TEXT("Default duplicate rejected"), S->OpenInspectionFromRequest(R));
	TestEqual(TEXT("Rejection preserves active state"), S->GetInspectionState(), EJMItemInspectionState::Inspecting);
	FJMInspectionTestAccessor::SetState(S, EJMItemInspectionState::Closed);
	R.InspectionData = nullptr;
	TestFalse(TEXT("Missing data fails safely"), S->OpenInspectionFromRequest(R));

	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	APlayerController* PC = World->SpawnActor<APlayerController>();
	AActor* Actor = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Controller"), PC) || !TestNotNull(TEXT("Source"), Actor)) return false;
	PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
	FJMInspectionTestAccessor::Block(S, PC); FJMInspectionTestAccessor::Restore(S);
	TestTrue(TEXT("Preexisting movement lock preserved"), PC->IsMoveInputIgnored());
	TestTrue(TEXT("Preexisting look lock preserved"), PC->IsLookInputIgnored());
	PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false);
	FJMInspectionTestAccessor::Block(S, PC);
	TestTrue(TEXT("Inspection movement lock applied"), PC->IsMoveInputIgnored());
	FJMInspectionTestAccessor::Restore(S);
	TestFalse(TEXT("Owned movement lock released"), PC->IsMoveInputIgnored());
	TestFalse(TEXT("Owned look lock released"), PC->IsLookInputIgnored());
	FJMInspectionTestAccessor::OwnSource(S, Actor);
	TestTrue(TEXT("Source hidden"), Actor->IsHidden());
	FJMInspectionTestAccessor::RestoreSource(S);
	TestFalse(TEXT("Source restored"), Actor->IsHidden());
	Actor->SetActorHiddenInGame(true);
	FJMInspectionTestAccessor::OwnSource(S, Actor); FJMInspectionTestAccessor::RestoreSource(S);
	TestTrue(TEXT("Originally hidden source remains hidden"), Actor->IsHidden());
	PC->Destroy(); Actor->Destroy();
	return true;
}
#endif
