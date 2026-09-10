#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "CameraSystem/BalhwajeomCameraTargetInterface.h"
#include "CameraSystem/BalhwajeomEvidenceActor.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
	constexpr double FocusTimeoutSeconds = 5.0;
	const FString AutomationSaveSlot = TEXT("BalhwajeomInvestigation_Automation");

	class FResetAutomationPhotoGalleryCommand final : public IAutomationLatentCommand
	{
	public:
		virtual bool Update() override
		{
			UGameplayStatics::DeleteGameInSlot(AutomationSaveSlot, 0);
			const FString AutomationPhotoDirectory = FPaths::Combine(
				FPaths::ProjectSavedDir(), TEXT("Investigation"), TEXT("Photos"), TEXT("Automation"));
			IFileManager::Get().Delete(
				*FPaths::Combine(AutomationPhotoDirectory, TEXT("PHOTO_PIG_MIRROR.png")),
				false, true);
			IFileManager::Get().Delete(
				*FPaths::Combine(AutomationPhotoDirectory, TEXT("PHOTO_SNOW_GLOBE.png")),
				false, true);
			return true;
		}
	};

	class FVerifyPrototypeCenteredCaptureCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FVerifyPrototypeCenteredCaptureCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			if (!GEditor || !GEditor->PlayWorld)
			{
				return false;
			}

			UWorld* World = GEditor->PlayWorld;
			if (!PhotoCamera.IsValid() || EvidenceTargets.Num() != 2)
			{
				ABalhwajeomCameraCharacter* Character = Cast<ABalhwajeomCameraCharacter>(
					UGameplayStatics::GetPlayerPawn(World, 0));
				if (!Character)
				{
					Test->AddError(TEXT("Prototype PIE did not spawn BP_OrbitViewCharacter."));
					return true;
				}

				ABalhwajeomEvidenceActor* Mirror = nullptr;
				ABalhwajeomEvidenceActor* SnowGlobe = nullptr;
				for (TActorIterator<ABalhwajeomEvidenceActor> It(World); It; ++It)
				{
					if (It->GetObjectID() == TEXT("OBJ_PIG_MIRROR"))
					{
						Mirror = *It;
					}
					else if (It->GetObjectID() == TEXT("OBJ_SNOW_GLOBE"))
					{
						SnowGlobe = *It;
					}
				}
				if (Mirror && SnowGlobe)
				{
					EvidenceTargets = {Mirror, SnowGlobe};
				}

				PhotoCamera = Character->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
				InvestigationSubsystem = World->GetGameInstance()
					? World->GetGameInstance()->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
					: nullptr;
				Controller = Cast<APlayerController>(Character->GetController());
				TArray<UCameraComponent*> Cameras;
				Character->GetComponents<UCameraComponent>(Cameras);
				for (UCameraComponent* Camera : Cameras)
				{
					if (Camera && Camera->GetName() == TEXT("FirstPersonCamera"))
					{
						FirstPersonCamera = Camera;
						break;
					}
				}

				if (!PhotoCamera.IsValid() || !InvestigationSubsystem.IsValid() ||
					EvidenceTargets.Num() != 2 ||
					!Controller.IsValid() || !FirstPersonCamera.IsValid())
				{
					Test->AddError(TEXT("Prototype camera/evidence setup is incomplete."));
					return true;
				}

				// Target acquisition must not depend on the visible mesh's collision asset.
				for (const TWeakObjectPtr<ABalhwajeomEvidenceActor>& Evidence : EvidenceTargets)
				{
					if (UStaticMeshComponent* VisibleMesh =
						Evidence->FindComponentByClass<UStaticMeshComponent>())
					{
						VisibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					}
				}

				const FVector FocusLocation =
					IBalhwajeomCameraTargetInterface::Execute_RequestCameraFocusLocation(
						EvidenceTargets[0].Get());
				Controller->SetControlRotation(
					(FocusLocation - FirstPersonCamera->GetComponentLocation()).Rotation());
				PhotoCamera->ToggleCameraMode();
				StageStartedAt = FPlatformTime::Seconds();
				return false;
			}

			if (!PhotoCamera->IsInCameraMode() || PhotoCamera->IsCameraTransitioning())
			{
				if (FPlatformTime::Seconds() - StageStartedAt > FocusTimeoutSeconds)
				{
					Test->AddError(TEXT("Photo camera mode did not finish entering."));
					return true;
				}
				return false;
			}

			ABalhwajeomEvidenceActor* Evidence = EvidenceTargets[TargetIndex].Get();
			const FVector FocusLocation =
				IBalhwajeomCameraTargetInterface::Execute_RequestCameraFocusLocation(Evidence);
			Controller->SetControlRotation(
				(FocusLocation - FirstPersonCamera->GetComponentLocation()).Rotation());

			if (PhotoCamera->GetActiveFocusTarget() != Evidence)
			{
				if (FPlatformTime::Seconds() - StageStartedAt > FocusTimeoutSeconds)
				{
					Test->AddError(FString::Printf(
						TEXT("Centered target '%s' was not acquired as the active focus target."),
						*Evidence->GetObjectID().ToString()));
					return true;
				}
				return false;
			}

			const FName ExpectedPhotoID = TargetIndex == 0
				? FName(TEXT("PHOTO_PIG_MIRROR")) : FName(TEXT("PHOTO_SNOW_GLOBE"));

			if (!bCaptureRequested)
			{
				FBalhwajeomCameraTargetInfo TargetInfo;
				if (!IBalhwajeomCameraTargetInterface::Execute_RequestCameraTargetInfo(
					Evidence, TargetInfo))
				{
					Test->AddError(TEXT("Focused evidence did not return camera target information."));
					return true;
				}

				Test->TestTrue(TEXT("Centered evidence is capture-enabled"), TargetInfo.bCanCapture);
				Test->TestEqual(TEXT("Centered evidence resolves the expected photo"),
					TargetInfo.PhotoID, ExpectedPhotoID);
				PhotoCamera->TakePhoto();
				bCaptureRequested = true;
				StageStartedAt = FPlatformTime::Seconds();
				return false;
			}

			if (!InvestigationSubsystem->HasCapturedPhoto(ExpectedPhotoID))
			{
				if (FPlatformTime::Seconds() - StageStartedAt > FocusTimeoutSeconds)
				{
					Test->AddError(FString::Printf(
						TEXT("Screenshot for '%s' was not saved and registered."),
						*ExpectedPhotoID.ToString()));
					return true;
				}
				return false;
			}

			TArray<FCapturedPhotoRecord> CapturedPhotos;
			InvestigationSubsystem->GetCapturedPhotos(CapturedPhotos);
			const FCapturedPhotoRecord* Record = CapturedPhotos.FindByPredicate(
				[ExpectedPhotoID](const FCapturedPhotoRecord& Candidate)
				{
					return Candidate.PhotoID == ExpectedPhotoID;
				});
			if (!Record)
			{
				Test->AddError(TEXT("Captured photo record was not available after registration."));
				return true;
			}

			const FString AbsoluteImagePath = FPaths::ConvertRelativePathToFull(
				FPaths::Combine(FPaths::ProjectSavedDir(), Record->ImageRelativePath));
			if (IFileManager::Get().FileSize(*AbsoluteImagePath) <= 0)
			{
				Test->AddError(FString::Printf(
					TEXT("Registered photo image is missing or empty: %s"),
					*AbsoluteImagePath));
				return true;
			}

			FImage SavedImage;
			if (!FImageUtils::LoadImage(*AbsoluteImagePath, SavedImage))
			{
				Test->AddError(FString::Printf(
					TEXT("Saved photo could not be decoded: %s"), *AbsoluteImagePath));
				return true;
			}

			int64 SampleCount = 0;
			int64 NonWhiteSampleCount = 0;
			for (int32 Y = 0; Y < SavedImage.SizeY; Y += 16)
			{
				for (int32 X = 0; X < SavedImage.SizeX; X += 16)
				{
					const FLinearColor Pixel = SavedImage.GetOnePixelLinear(X, Y);
					++SampleCount;
					if (Pixel.R < 0.95f || Pixel.G < 0.95f || Pixel.B < 0.95f)
					{
						++NonWhiteSampleCount;
					}
				}
			}
			if (SampleCount == 0 || NonWhiteSampleCount * 100 < SampleCount)
			{
				Test->AddError(FString::Printf(
					TEXT("Saved photo is blank/white: %s"), *AbsoluteImagePath));
				return true;
			}

			Test->AddInfo(FString::Printf(
				TEXT("Centered %s saved and registered as %s at %s."),
				*Evidence->GetObjectID().ToString(), *ExpectedPhotoID.ToString(),
				*AbsoluteImagePath));

			++TargetIndex;
			bCaptureRequested = false;
			if (TargetIndex >= EvidenceTargets.Num())
			{
				return true;
			}
			StageStartedAt = FPlatformTime::Seconds();
			return false;
		}

	private:
		FAutomationTestBase* Test = nullptr;
		TWeakObjectPtr<UBalhwajeomPhotoCameraComponent> PhotoCamera;
		TWeakObjectPtr<UBalhwajeomInvestigationSubsystem> InvestigationSubsystem;
		TWeakObjectPtr<APlayerController> Controller;
		TWeakObjectPtr<UCameraComponent> FirstPersonCamera;
		TArray<TWeakObjectPtr<ABalhwajeomEvidenceActor>> EvidenceTargets;
		double StageStartedAt = 0.0;
		int32 TargetIndex = 0;
		bool bCaptureRequested = false;
	};

	class FVerifyPersistentGalleryAfterRestartCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FVerifyPersistentGalleryAfterRestartCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			if (!GEditor || !GEditor->PlayWorld)
			{
				return false;
			}

			UGameInstance* GameInstance = GEditor->PlayWorld->GetGameInstance();
			UBalhwajeomInvestigationSubsystem* Investigation = GameInstance
				? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>() : nullptr;
			if (!Investigation)
			{
				Test->AddError(TEXT("Restarted PIE has no investigation subsystem."));
				return true;
			}

			Test->TestTrue(TEXT("PigMirror photo survives PIE restart"),
				Investigation->HasCapturedPhoto(TEXT("PHOTO_PIG_MIRROR")));
			Test->TestTrue(TEXT("SnowGlobe photo survives PIE restart"),
				Investigation->HasCapturedPhoto(TEXT("PHOTO_SNOW_GLOBE")));

			TArray<FCapturedPhotoRecord> Photos;
			Investigation->GetCapturedPhotos(Photos);
			Test->TestEqual(TEXT("Persistent gallery restores both records"), Photos.Num(), 2);
			for (const FCapturedPhotoRecord& Photo : Photos)
			{
				const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
					FPaths::Combine(FPaths::ProjectSavedDir(), Photo.ImageRelativePath));
				Test->TestTrue(FString::Printf(TEXT("Restored image exists: %s"), *Photo.PhotoID.ToString()),
					IFileManager::Get().FileSize(*AbsolutePath) > 0);
			}
			Test->AddInfo(TEXT("Persistent photo gallery restored after a complete PIE restart."));
			return true;
		}

	private:
		FAutomationTestBase* Test = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationPrototypeCenteredFocusTest,
	"Balhwajeom.Camera.PrototypeCenteredFocus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationPrototypeCenteredFocusTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FResetAutomationPhotoGalleryCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(
		TEXT("/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyPrototypeCenteredCaptureCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyPersistentGalleryAfterRestartCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FResetAutomationPhotoGalleryCommand());
	return true;
}

#endif
