#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Interaction/InspectionComponent.h"
#include "InputAction.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionDistanceStateTest,
	"Balhwajeom.Interaction.Player.DistanceState",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionDistanceStateTest::RunTest(const FString& Parameters)
{
	constexpr float CloseDistance = 300.0f;
	constexpr float MiddleDistance = 700.0f;
	constexpr float MaxDisplayDistance = 1500.0f;

	// Close
	TestTrue(
		TEXT("Distance 0 should be Close"),
		UPlayerInteractionComponent::ClassifyDistance(
			0.0f,
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Close
	);

	TestTrue(
		TEXT("Distance 300 should be Close"),
		UPlayerInteractionComponent::ClassifyDistance(
			300.0f,
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Close
	);

	// Middle
	TestTrue(
		TEXT("Distance 301 should be Middle"),
		UPlayerInteractionComponent::ClassifyDistance(
			301.0f,
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Middle
	);

	TestTrue(
		TEXT("Distance 700 should be Middle"),
		UPlayerInteractionComponent::ClassifyDistance(
			700.0f,
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Middle
	);

	// Far
	TestTrue(
		TEXT("Distance 701 should be Far"),
		UPlayerInteractionComponent::ClassifyDistance(
			701.0f,
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Far
	);

	TestTrue(
		TEXT("Distance 1500 should be Far"),
		UPlayerInteractionComponent::ClassifyDistance(
			1500.0f,
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Far
	);

	// Out of range
	TestTrue(
		TEXT("Distance 1501 should be OutOfRange"),
		UPlayerInteractionComponent::ClassifyDistance(
			1501.0f,
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::OutOfRange
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionDistanceBetweenPointsTest,
	"Balhwajeom.Interaction.Player.DistanceBetweenPoints",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionDistanceBetweenPointsTest::RunTest(
	const FString& Parameters
)
{
	constexpr float CloseDistance = 300.0f;
	constexpr float MiddleDistance = 700.0f;
	constexpr float MaxDisplayDistance = 1500.0f;

	const FVector PlayerLocation(0.0f, 0.0f, 0.0f);

	TestTrue(
		TEXT("Target at 300 units should be Close"),
		UPlayerInteractionComponent::ClassifyDistanceBetweenPoints(
			PlayerLocation,
			FVector(300.0f, 0.0f, 0.0f),
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Close
	);

	TestTrue(
		TEXT("Target at 500 units should be Middle"),
		UPlayerInteractionComponent::ClassifyDistanceBetweenPoints(
			PlayerLocation,
			FVector(300.0f, 400.0f, 0.0f),
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Middle
	);

	TestTrue(
		TEXT("Target at 1000 units should be Far"),
		UPlayerInteractionComponent::ClassifyDistanceBetweenPoints(
			PlayerLocation,
			FVector(600.0f, 800.0f, 0.0f),
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::Far
	);

	TestTrue(
		TEXT("Target beyond maximum distance should be OutOfRange"),
		UPlayerInteractionComponent::ClassifyDistanceBetweenPoints(
			PlayerLocation,
			FVector(1600.0f, 0.0f, 0.0f),
			CloseDistance,
			MiddleDistance,
			MaxDisplayDistance
		) == EPlayerInspectionDistanceState::OutOfRange
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionDistanceStateCacheTest,
	"Balhwajeom.Interaction.Player.DistanceStateCache",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionDistanceStateCacheTest::RunTest(
	const FString& Parameters
)
{
	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>();

	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>();

	Inspection->CloseDistance = 300.0f;
	Inspection->MiddleDistance = 700.0f;
	Inspection->MaxDisplayDistance = 1500.0f;

	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(500.0f, 0.0f, 0.0f)
	);

	TestTrue(
		TEXT("Target at 500 units should be stored as Middle"),
		PlayerInteraction->GetDistanceStateForInspectable(Inspection)
		== EPlayerInspectionDistanceState::Middle
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionDistanceStateChangeTest,
	"Balhwajeom.Interaction.Player.DistanceStateChange",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionDistanceStateChangeTest::RunTest(
	const FString& Parameters
)
{
	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>();

	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>();

	Inspection->CloseDistance = 300.0f;
	Inspection->MiddleDistance = 700.0f;
	Inspection->MaxDisplayDistance = 1500.0f;

	const FVector PlayerLocation = FVector::ZeroVector;

	const bool bFirstUpdateChanged =
		PlayerInteraction->UpdateDistanceStateForInspectable(
			Inspection,
			PlayerLocation,
			FVector(500.0f, 0.0f, 0.0f)
		);

	TestTrue(
		TEXT("First distance state should count as changed"),
		bFirstUpdateChanged
	);

	const bool bSameStateChanged =
		PlayerInteraction->UpdateDistanceStateForInspectable(
			Inspection,
			PlayerLocation,
			FVector(600.0f, 0.0f, 0.0f)
		);

	TestFalse(
		TEXT("Remaining in Middle should not count as changed"),
		bSameStateChanged
	);

	const bool bCloseStateChanged =
		PlayerInteraction->UpdateDistanceStateForInspectable(
			Inspection,
			PlayerLocation,
			FVector(200.0f, 0.0f, 0.0f)
		);

	TestTrue(
		TEXT("Changing from Middle to Close should count as changed"),
		bCloseStateChanged
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionCanInspectTest,
	"Balhwajeom.Interaction.Player.CanInspect",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionCanInspectTest::RunTest(
	const FString& Parameters
)
{
	TestFalse(
		TEXT("OutOfRange should not be inspectable"),
		UPlayerInteractionComponent::CanInspectDistanceState(
			EPlayerInspectionDistanceState::OutOfRange
		)
	);

	TestFalse(
		TEXT("Far should not be inspectable"),
		UPlayerInteractionComponent::CanInspectDistanceState(
			EPlayerInspectionDistanceState::Far
		)
	);

	TestFalse(
		TEXT("Middle should not be inspectable"),
		UPlayerInteractionComponent::CanInspectDistanceState(
			EPlayerInspectionDistanceState::Middle
		)
	);

	TestTrue(
		TEXT("Close should be inspectable"),
		UPlayerInteractionComponent::CanInspectDistanceState(
			EPlayerInspectionDistanceState::Close
		)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionFocusedInspectionTest,
	"Balhwajeom.Interaction.Player.FocusedInspection",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionFocusedInspectionTest::RunTest(
	const FString& Parameters
)
{
	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>();

	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>();

	Inspection->CloseDistance = 300.0f;
	Inspection->MiddleDistance = 700.0f;
	Inspection->MaxDisplayDistance = 1500.0f;

	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(200.0f, 0.0f, 0.0f)
	);

	TestTrue(
		TEXT("Close hit inspection should be accepted as focused"),
		PlayerInteraction->ResolveFocusedInspection(Inspection)
		== Inspection
	);

	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(500.0f, 0.0f, 0.0f)
	);

	TestNull(
		TEXT("Middle hit inspection should not be accepted as focused"),
		PlayerInteraction->ResolveFocusedInspection(Inspection)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionTryInspectTest,
	"Balhwajeom.Interaction.Player.TryInspect",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionTryInspectTest::RunTest(
	const FString& Parameters
)
{
	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>();

	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>();

	Inspection->CloseDistance = 300.0f;
	Inspection->MiddleDistance = 700.0f;
	Inspection->MaxDisplayDistance = 1500.0f;
	Inspection->InspectionText =
		FText::FromString(TEXT("Test Inspection Text"));

	// Close 상태로 만든다.
	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(200.0f, 0.0f, 0.0f)
	);

	PlayerInteraction->SetFocusedInspection(Inspection);

	FText ResultText;

	const bool bInspected =
		PlayerInteraction->TryInspect(ResultText);

	TestTrue(
		TEXT("Focused Close inspection should be inspectable"),
		bInspected
	);

	TestEqual(
		TEXT("Inspection text should match"),
		ResultText.ToString(),
		FString(TEXT("Test Inspection Text"))
	);

	// 같은 대상을 Middle 거리로 변경한다.
	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(500.0f, 0.0f, 0.0f)
	);

	ResultText = FText::GetEmpty();

	const bool bMiddleInspected =
		PlayerInteraction->TryInspect(ResultText);

	TestFalse(
		TEXT("Focused Middle inspection should not be inspectable"),
		bMiddleInspected
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionRequestInspectTest,
	"Balhwajeom.Interaction.Player.RequestInspect",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionRequestInspectTest::RunTest(
	const FString& Parameters
)
{
	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>();

	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>();

	Inspection->CloseDistance = 300.0f;
	Inspection->MiddleDistance = 700.0f;
	Inspection->MaxDisplayDistance = 1500.0f;
	Inspection->InspectionText =
		FText::FromString(TEXT("Test Inspection Text"));

	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(200.0f, 0.0f, 0.0f)
	);

	PlayerInteraction->SetFocusedInspection(Inspection);

	TestTrue(
		TEXT("RequestInspect should succeed for a focused Close target"),
		PlayerInteraction->RequestInspect()
	);

	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(500.0f, 0.0f, 0.0f)
	);

	TestFalse(
		TEXT("RequestInspect should fail outside Close range"),
		PlayerInteraction->RequestInspect()
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionInputStartedTest,
	"Balhwajeom.Interaction.Player.InputStarted",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionInputStartedTest::RunTest(
	const FString& Parameters
)
{
	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>();

	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>();

	Inspection->CloseDistance = 300.0f;
	Inspection->MiddleDistance = 700.0f;
	Inspection->MaxDisplayDistance = 1500.0f;
	Inspection->InspectionText =
		FText::FromString(TEXT("Test Inspection Text"));

	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(200.0f, 0.0f, 0.0f)
	);

	PlayerInteraction->SetFocusedInspection(Inspection);

	// Input Action이 설정되지 않았다면
	// Input 경로에서는 아무것도 실행하지 않아야 한다.
	TestFalse(
		TEXT("Missing InteractAction should not trigger inspection"),
		PlayerInteraction->HandleInteractStarted()
	);

	PlayerInteraction->InteractAction =
		NewObject<UInputAction>();

	TestTrue(
		TEXT("Configured interaction input should request inspection"),
		PlayerInteraction->HandleInteractStarted()
	);

	return true;
}

#endif