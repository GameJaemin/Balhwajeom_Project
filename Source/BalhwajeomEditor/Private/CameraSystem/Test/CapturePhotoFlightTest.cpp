#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CameraSystem/BalhwajeomCapturePhotoWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/VerticalBox.h"
#include "Editor.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCapturePhotoMultilineSentenceTest,
	"Balhwajeom.Camera.CapturePhotoMultilineSentence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapturePhotoMultilineSentenceTest::RunTest(const FString& Parameters)
{
	UClass* WidgetClass = LoadClass<UBalhwajeomCapturePhotoWidget>(nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_CapturePhoto.WBP_CapturePhoto_C"));
	auto* Widget = WidgetClass && GEditor
		? CreateWidget<UBalhwajeomCapturePhotoWidget>(
			GEditor->GetEditorWorldContext().World(), WidgetClass)
		: nullptr;
	if (!TestNotNull(TEXT("Live capture widget"), Widget)) return false;
	Widget->AddToRoot();
	Widget->PresentCapture(
		nullptr,
		FText::FromString(TEXT("Before [] text.\n[] after.\n\nLast [].")),
		{},
		true);

	int32 ForcedLineBreakCount = 0;
	for (int32 ChildIndex = 0;
		Widget->SentenceBuilder && ChildIndex < Widget->SentenceBuilder->GetChildrenCount();
		++ChildIndex)
	{
		UWidget* Child = Widget->SentenceBuilder->GetChildAt(ChildIndex);
		if (const UTextBlock* Segment = Cast<UTextBlock>(Child))
		{
			TestFalse(TEXT("Generated sentence fragments contain no embedded newline"),
				Segment->GetText().ToString().Contains(TEXT("\n")) ||
				Segment->GetText().ToString().Contains(TEXT("\r")));
		}
		if (const UWrapBoxSlot* Slot = Child ? Cast<UWrapBoxSlot>(Child->Slot) : nullptr)
		{
			ForcedLineBreakCount += Slot->DoesForceNewLine() ? 1 : 0;
		}
	}
	TestEqual(TEXT("Every authored newline becomes a WrapBox line break"),
		ForcedLineBreakCount, 3);
	Widget->RemoveFromRoot();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCapturePhotoFlightTest,
	"Balhwajeom.Camera.CapturePhotoFlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapturePhotoFlightTest::RunTest(const FString& Parameters)
{
	UClass* WidgetClass = LoadClass<UBalhwajeomCapturePhotoWidget>(nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_CapturePhoto.WBP_CapturePhoto_C"));
	auto* Widget = WidgetClass && GEditor
		? CreateWidget<UBalhwajeomCapturePhotoWidget>(GEditor->GetEditorWorldContext().World(), WidgetClass)
		: nullptr;
	if (!TestNotNull(TEXT("Live capture widget"), Widget)) return false;
	Widget->AddToRoot();
	URetainerBox* CardComposite = Cast<URetainerBox>(
		Widget->GetWidgetFromName(TEXT("CardComposite")));
	if (!TestNotNull(TEXT("Card visuals are composited before animation"), CardComposite))
	{
		Widget->RemoveFromRoot();
		return false;
	}

	TArray<FText> Words;
	Words.Add(FText::FromString(TEXT("Keyword 1")));
	Words.Add(FText::FromString(TEXT("Keyword 2")));
	Words.Add(FText::FromString(TEXT("Keyword 3")));
	Widget->PresentCapture(
		nullptr,
		FText::FromString(TEXT("Sentence")),
		Words,
		true);

	TestTrue(TEXT("AE presentation can start without TAB target geometry"), Widget->IsPresentationReady());
	TestTrue(TEXT("extended AE timeline ends at frame 93"),
		FMath::IsNearlyEqual(Widget->GetAnimationDuration(), 93.0f / 60.0f));
	TestEqual(TEXT("one animated widget exists per granted keyword"),
		Widget->KeywordList->GetChildrenCount(), 3);
	TestTrue(TEXT("exit distance places the moving content below the viewport"),
		FMath::IsNearlyEqual(
			UBalhwajeomCapturePhotoWidget::CalculateExitDistance(
				1080.0f,
				240.0f,
				360.0f,
				32.0f),
			872.0f));

	auto ApplyAtFrame = [Widget](const float Frame)
	{
		Widget->ApplyPresentationTimeline((Frame / 60.0f) / Widget->GetAnimationDuration());
	};

	ApplyAtFrame(0.0f);
	const FWidgetTransform CardStart = CardComposite->GetRenderTransform();
	TestTrue(TEXT("card enters from the right"), CardStart.Translation.X > 0.0f);
	TestTrue(TEXT("card starts at the AE quarter turn"),
		FMath::IsNearlyEqual(CardStart.Angle, 90.0f));
	TestTrue(TEXT("AE reference scale is ignored"),
		CardStart.Scale.Equals(FVector2D(1.0f, 1.0f)));
	TestEqual(TEXT("card is visible from its first frame"), CardComposite->GetRenderOpacity(), 1.0f);
	TestEqual(TEXT("screen dimmer stays fully visible during entry"),
		Widget->ScreenDimmer->GetRenderOpacity(), 1.0f);
	const float Keyword1StartX =
		Widget->KeywordList->GetChildAt(0)->GetRenderTransform().Translation.X;
	for (int32 KeywordIndex = 0; KeywordIndex < 3; ++KeywordIndex)
	{
		UWidget* Keyword = Widget->KeywordList->GetChildAt(KeywordIndex);
		TestTrue(TEXT("keyword starts to the right"), Keyword->GetRenderTransform().Translation.X > 0.0f);
		TestTrue(TEXT("keyword starts rotated"),
			FMath::IsNearlyEqual(Keyword->GetRenderTransform().Angle, 90.0f));
		TestEqual(TEXT("keyword starts transparent"), Keyword->GetRenderOpacity(), 0.0f);
	}

	auto TestAeEntrySample = [this, CardStart](
		const TCHAR* What,
		const FWidgetTransform& Transform,
		const float ExpectedRemainingRatio)
	{
		TestTrue(What,
			FMath::IsNearlyEqual(
				Transform.Translation.X / CardStart.Translation.X,
				ExpectedRemainingRatio,
				0.002f) &&
			FMath::IsNearlyEqual(
				Transform.Angle / CardStart.Angle,
				ExpectedRemainingRatio,
				0.002f));
	};

	ApplyAtFrame(6.25f);
	TestAeEntrySample(
		TEXT("card matches AE temporal bezier at 25 percent"),
		CardComposite->GetRenderTransform(),
		0.3094796f);

	ApplyAtFrame(12.5f);
	TestAeEntrySample(
		TEXT("card matches AE temporal bezier at 50 percent"),
		CardComposite->GetRenderTransform(),
		0.1101237f);

	ApplyAtFrame(18.75f);
	TestAeEntrySample(
		TEXT("card matches AE temporal bezier at 75 percent"),
		CardComposite->GetRenderTransform(),
		0.0235550f);

	ApplyAtFrame(17.5f);
	const FWidgetTransform Keyword1MidEntry =
		Widget->KeywordList->GetChildAt(0)->GetRenderTransform();
	TestTrue(TEXT("keywords use the same AE temporal bezier"),
		FMath::IsNearlyEqual(
			Keyword1MidEntry.Translation.X / Keyword1StartX,
			0.1101237f,
			0.002f) &&
		FMath::IsNearlyEqual(
			Keyword1MidEntry.Angle / CardStart.Angle,
			0.1101237f,
			0.002f));

	ApplyAtFrame(20.0f);
	TestTrue(TEXT("card is still moving after the former 20-frame entrance"),
		!CardComposite->GetRenderTransform().Translation.IsNearlyZero() &&
		!FMath::IsNearlyZero(CardComposite->GetRenderTransform().Angle));

	ApplyAtFrame(25.0f);
	TestTrue(TEXT("card finishes its 25-frame entrance"),
		CardComposite->GetRenderTransform().Translation.IsNearlyZero() &&
		FMath::IsNearlyZero(CardComposite->GetRenderTransform().Angle));
	TestTrue(TEXT("card never scales during entry"),
		CardComposite->GetRenderTransform().Scale.Equals(FVector2D(1.0f, 1.0f)));

	ApplyAtFrame(14.0f);
	TestTrue(TEXT("second keyword waits for its ten-frame interval"),
		FMath::IsNearlyEqual(
			Widget->KeywordList->GetChildAt(1)->GetRenderTransform().Translation.X,
			Keyword1StartX));
	ApplyAtFrame(24.0f);
	TestTrue(TEXT("third keyword waits for its ten-frame interval"),
		FMath::IsNearlyEqual(
			Widget->KeywordList->GetChildAt(2)->GetRenderTransform().Translation.X,
			Keyword1StartX));

	ApplyAtFrame(30.0f);
	const float Keyword1X = Widget->KeywordList->GetChildAt(0)->GetRenderTransform().Translation.X;
	const float Keyword2X = Widget->KeywordList->GetChildAt(1)->GetRenderTransform().Translation.X;
	const float Keyword3X = Widget->KeywordList->GetChildAt(2)->GetRenderTransform().Translation.X;
	TestTrue(TEXT("keywords follow at ten-frame intervals"),
		Keyword1X < Keyword2X && Keyword2X < Keyword3X);

	ApplyAtFrame(50.0f);
	for (int32 KeywordIndex = 0; KeywordIndex < 3; ++KeywordIndex)
	{
		UWidget* Keyword = Widget->KeywordList->GetChildAt(KeywordIndex);
		TestTrue(TEXT("every keyword finishes in its authored layout slot"),
			Keyword->GetRenderTransform().Translation.IsNearlyZero());
		TestEqual(TEXT("every keyword is opaque after entering"), Keyword->GetRenderOpacity(), 1.0f);
	}

	auto TestAeExitPosition = [this, Widget](
		const TCHAR* What,
		const float ExpectedProgress)
	{
		TestTrue(What,
			FMath::IsNearlyEqual(
				Widget->CardRoot->GetRenderTransform().Translation.Y /
					Widget->ExitOffset.Y,
				ExpectedProgress,
				0.002f));
	};

	ApplyAtFrame(58.0f);
	TestTrue(TEXT("downward exit waits for the extended entry sequence"),
		Widget->CardRoot->GetRenderTransform().Translation.IsNearlyZero());

	ApplyAtFrame(74.25f);
	TestAeExitPosition(
		TEXT("exit position matches AE temporal bezier at 25 percent"),
		0.0308012f);
	TestEqual(TEXT("card remains opaque throughout the downward exit"),
		CardComposite->GetRenderOpacity(), 1.0f);

	ApplyAtFrame(80.5f);
	TestAeExitPosition(
		TEXT("exit position matches AE temporal bezier at 50 percent"),
		0.1465657f);
	TestEqual(TEXT("card remains opaque at the exit midpoint"),
		CardComposite->GetRenderOpacity(), 1.0f);
	TestEqual(TEXT("screen dimmer remains fixed until frame 83"),
		Widget->ScreenDimmer->GetRenderOpacity(), 1.0f);

	ApplyAtFrame(86.75f);
	TestAeExitPosition(
		TEXT("exit position matches AE temporal bezier at 75 percent"),
		0.4232377f);
	TestEqual(TEXT("composited card does not fade during exit"),
		CardComposite->GetRenderOpacity(), 1.0f);
	TestEqual(TEXT("photo does not fade independently"),
		Widget->CapturedPhotoImage->GetRenderOpacity(), 1.0f);
	TestEqual(TEXT("card background does not fade independently"),
		Widget->CardBackground->GetRenderOpacity(), 1.0f);
	TestEqual(TEXT("sentence does not fade independently"),
		Widget->SentenceBackground->GetRenderOpacity(), 1.0f);
	TestTrue(TEXT("screen dimmer keeps its independent linear fade"),
		FMath::IsNearlyEqual(
			Widget->ScreenDimmer->GetRenderOpacity(), 0.625f, 0.002f) &&
		Widget->ScreenDimmer->GetRenderTransform().Translation.IsNearlyZero());
	for (int32 KeywordIndex = 0; KeywordIndex < 3; ++KeywordIndex)
	{
		TestEqual(TEXT("each keyword remains opaque during exit"),
			Widget->KeywordList->GetChildAt(KeywordIndex)->GetRenderOpacity(), 1.0f);
	}

	ApplyAtFrame(88.0f);
	TestEqual(TEXT("card stays opaque while the dimmer reaches half opacity"),
		CardComposite->GetRenderOpacity(), 1.0f);
	TestTrue(TEXT("screen dimmer fades over the final ten frames"),
		FMath::IsNearlyEqual(Widget->ScreenDimmer->GetRenderOpacity(), 0.5f, 0.002f));

	ApplyAtFrame(93.0f);
	TestTrue(TEXT("shared root completes the downward exit"),
		Widget->CardRoot->GetRenderTransform().Translation.Y > 0.0f);
	TestEqual(TEXT("composited card remains opaque at the end"),
		CardComposite->GetRenderOpacity(), 1.0f);
	TestEqual(TEXT("screen dimmer is fully faded at the end"),
		Widget->ScreenDimmer->GetRenderOpacity(), 0.0f);
	for (int32 KeywordIndex = 0; KeywordIndex < 3; ++KeywordIndex)
	{
		TestEqual(TEXT("each keyword remains opaque at the end"),
			Widget->KeywordList->GetChildAt(KeywordIndex)->GetRenderOpacity(), 1.0f);
	}

	Widget->RemoveFromRoot();
	return !HasAnyErrors();
}
#endif
