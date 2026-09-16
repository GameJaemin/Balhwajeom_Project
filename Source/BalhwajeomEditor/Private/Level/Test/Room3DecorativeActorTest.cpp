#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomEvidenceActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoom3BPTestAll12IsDecorativeStaticMeshTest,
	"Balhwajeom.Level.Room3.BPTestAll12IsDecorativeStaticMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoom3BPTestAll12IsDecorativeStaticMeshTest::RunTest(
	const FString& Parameters)
{
	UWorld* Room3 = LoadObject<UWorld>(
		nullptr,
		TEXT("/Game/Levels/room3.room3"));
	if (!TestNotNull(TEXT("room3 level should load"), Room3) ||
		!TestNotNull(
			TEXT("room3 should have a persistent level"),
			Room3->PersistentLevel.Get()))
	{
		return false;
	}

	AActor* TargetActor = nullptr;
	for (AActor* Actor : Room3->PersistentLevel->Actors)
	{
		if (IsValid(Actor) && Actor->GetActorLabel() == TEXT("BP_TestAll12"))
		{
			TargetActor = Actor;
			break;
		}
	}

	if (!TestNotNull(TEXT("room3 should contain BP_TestAll12"), TargetActor))
	{
		return false;
	}

	TestFalse(
		TEXT("BP_TestAll12 should no longer be an investigation evidence actor"),
		TargetActor->IsA<ABalhwajeomEvidenceActor>());

	AStaticMeshActor* DecorativeMesh = Cast<AStaticMeshActor>(TargetActor);
	if (!TestNotNull(
		TEXT("BP_TestAll12 should be represented by a plain StaticMeshActor"),
		DecorativeMesh))
	{
		return false;
	}

	const UStaticMeshComponent* MeshComponent =
		DecorativeMesh->GetStaticMeshComponent();
	TestNotNull(TEXT("Decorative actor should have a static mesh component"), MeshComponent);
	if (MeshComponent)
	{
		TestNotNull(
			TEXT("Decorative actor should retain its visible static mesh"),
			MeshComponent->GetStaticMesh().Get());
	}

	return true;
}

#endif
