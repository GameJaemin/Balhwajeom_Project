#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/InspectionLabelPresentation.h"
#include "Interaction/InspectionComponent.h"
#include "Interaction/PlayerInteractionComponent.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionLabelPresentationEndpointsTest,
	"Balhwajeom.Interaction.Player.LabelPresentation.Endpoints",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionLabelPresentationEaseInOutTest,
	"Balhwajeom.Interaction.Player.LabelPresentation.EaseInOut",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionLabelPresentationDegenerateRangeTest,
	"Balhwajeom.Interaction.Player.LabelPresentation.DegenerateRange",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionLabelPresentationAppliesWholeWidgetTest,
	"Balhwajeom.Interaction.Player.LabelPresentation.AppliesWholeWidget",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionLabelPresentationAppliesObjectLabelActorTest,
	"Balhwajeom.Interaction.Player.LabelPresentation.AppliesObjectLabelActor",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionLabelPresentationReachesMinimumAtMiddleDistanceTest,
	"Balhwajeom.Interaction.Player.LabelPresentation.ReachesMinimumAtMiddleDistance",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionLabelPresentationEndpointsTest::RunTest(
	const FString& Parameters
)
{
	constexpr float MinimumPresentationDistance = 1500.0f;
	float Scale = 0.0f;
	float Opacity = 0.0f;

	BalhwajeomInspectionLabelPresentation::Calculate(
		0.0f,
		MinimumPresentationDistance,
		Scale,
		Opacity
	);
	TestEqual(TEXT("Closest label scale should be 100%"), Scale, 1.0f);
	TestEqual(TEXT("Closest label opacity should be 100%"), Opacity, 1.0f);

	BalhwajeomInspectionLabelPresentation::Calculate(
		MinimumPresentationDistance,
		MinimumPresentationDistance,
		Scale,
		Opacity
	);
	TestEqual(TEXT("Minimum presentation scale should be 25%"), Scale, 0.25f);
	TestEqual(TEXT("Minimum presentation opacity should be 5%"), Opacity, 0.05f);

	return true;
}


bool FPlayerInteractionLabelPresentationEaseInOutTest::RunTest(
	const FString& Parameters
)
{
	float Scale = 0.0f;
	float Opacity = 0.0f;

	BalhwajeomInspectionLabelPresentation::Calculate(
		375.0f,
		1500.0f,
		Scale,
		Opacity
	);

	TestEqual(
		TEXT("Quarter-distance scale should use ease in/out"),
		Scale,
		0.90625f,
		UE_KINDA_SMALL_NUMBER
	);
	TestEqual(
		TEXT("Quarter-distance opacity should use ease in/out"),
		Opacity,
		0.88125f,
		UE_KINDA_SMALL_NUMBER
	);

	return true;
}


bool FPlayerInteractionLabelPresentationDegenerateRangeTest::RunTest(
	const FString& Parameters
)
{
	float Scale = 0.0f;
	float Opacity = 0.0f;

	BalhwajeomInspectionLabelPresentation::Calculate(
		0.0f,
		-1.0f,
		Scale,
		Opacity
	);

	TestTrue(TEXT("Invalid maximum distance should produce a finite scale"), FMath::IsFinite(Scale));
	TestTrue(TEXT("Invalid maximum distance should produce a finite opacity"), FMath::IsFinite(Opacity));
	TestEqual(
		TEXT("Invalid maximum distance should use the safe far scale"),
		Scale,
		0.25f
	);
	TestEqual(
		TEXT("Invalid maximum distance should use the safe far opacity"),
		Opacity,
		0.05f
	);

	return true;
}


bool FPlayerInteractionLabelPresentationAppliesWholeWidgetTest::RunTest(
	const FString& Parameters
)
{
	UClass* LabelWidgetClass = LoadClass<UUserWidget>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel_C")
	);
	TestNotNull(TEXT("WBP_ObjectLabel class should load"), LabelWidgetClass);
	if (!LabelWidgetClass)
	{
		return false;
	}

	UUserWidget* LabelWidget = NewObject<UUserWidget>(
		GetTransientPackage(),
		LabelWidgetClass
	);
	TestNotNull(TEXT("Label widget should be constructible"), LabelWidget);
	if (!LabelWidget)
	{
		return false;
	}

	BalhwajeomInspectionLabelPresentation::ApplyToWidget(
		LabelWidget,
		375.0f,
		1500.0f
	);

	TestEqual(
		TEXT("The whole label should scale from its center"),
		LabelWidget->GetRenderTransformPivot(),
		FVector2D(0.5f, 0.5f)
	);
	TestEqual(
		TEXT("The whole label should receive the eased scale"),
		LabelWidget->GetRenderTransform().Scale,
		FVector2D(0.90625f, 0.90625f)
	);
	TestEqual(
		TEXT("The whole label should receive the eased opacity"),
		LabelWidget->GetRenderOpacity(),
		0.88125f,
		UE_KINDA_SMALL_NUMBER
	);

	return true;
}


bool FPlayerInteractionLabelPresentationAppliesObjectLabelActorTest::RunTest(
	const FString& Parameters
)
{
	UClass* LabelWidgetClass = LoadClass<UUserWidget>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel_C")
	);
	TestNotNull(TEXT("WBP_ObjectLabel class should load"), LabelWidgetClass);
	if (!LabelWidgetClass)
	{
		return false;
	}

	AActor* TargetActor = NewObject<AActor>();
	UWidgetComponent* LabelComponent = NewObject<UWidgetComponent>(TargetActor);
	UUserWidget* LabelWidget = NewObject<UUserWidget>(
		GetTransientPackage(),
		LabelWidgetClass
	);
	TargetActor->AddInstanceComponent(LabelComponent);
	LabelComponent->SetWidgetClass(LabelWidgetClass);
	LabelComponent->SetWidget(LabelWidget);
	LabelComponent->SetVisibility(true);

	BalhwajeomInspectionLabelPresentation::ApplyToActor(
		TargetActor,
		1500.0f,
		1500.0f
	);

	TestEqual(
		TEXT("Visible WBP_ObjectLabel component should receive far scale"),
		LabelWidget->GetRenderTransform().Scale,
		FVector2D(0.25f, 0.25f)
	);
	TestEqual(
		TEXT("Visible WBP_ObjectLabel component should receive far opacity"),
		LabelWidget->GetRenderOpacity(),
		0.05f,
		UE_KINDA_SMALL_NUMBER
	);

	return true;
}


bool FPlayerInteractionLabelPresentationReachesMinimumAtMiddleDistanceTest::RunTest(
	const FString& Parameters
)
{
	UClass* LabelWidgetClass = LoadClass<UUserWidget>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel_C")
	);
	TestNotNull(TEXT("WBP_ObjectLabel class should load"), LabelWidgetClass);
	if (!LabelWidgetClass)
	{
		return false;
	}

	AActor* TargetActor = NewObject<AActor>();
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>(TargetActor);
	UWidgetComponent* LabelComponent = NewObject<UWidgetComponent>(TargetActor);
	UUserWidget* LabelWidget = NewObject<UUserWidget>(
		GetTransientPackage(),
		LabelWidgetClass
	);
	TargetActor->AddInstanceComponent(Inspection);
	TargetActor->AddInstanceComponent(LabelComponent);
	Inspection->MiddleDistance = 700.0f;
	Inspection->MaxDisplayDistance = 1500.0f;
	LabelComponent->SetWidgetClass(LabelWidgetClass);
	LabelComponent->SetWidget(LabelWidget);
	LabelComponent->SetVisibility(true);

	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>();
	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(700.0f, 0.0f, 0.0f)
	);

	TestEqual(
		TEXT("Middle distance should apply minimum scale to the object label"),
		LabelWidget->GetRenderTransform().Scale,
		FVector2D(0.25f, 0.25f)
	);
	TestEqual(
		TEXT("Middle distance should apply minimum opacity to the object label"),
		LabelWidget->GetRenderOpacity(),
		0.05f,
		UE_KINDA_SMALL_NUMBER
	);

	return true;
}

#endif
