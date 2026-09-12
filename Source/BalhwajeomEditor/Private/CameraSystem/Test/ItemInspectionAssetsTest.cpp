#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CameraSystem/BalhwajeomCameraGameMode.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EnhancedActionKeyMapping.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "ItemInspection/JMInspectableComponent.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "ItemInspection/JMItemInspectionWidgetBase.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemInspectionAssetsTest, "Balhwajeom.ItemInspection.AssetsAndFixture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemInspectionAssetsTest::RunTest(const FString& Parameters)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/ItemInspector/Item/SM_OldKey.SM_OldKey"));
	UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/ItemInspector/ItemInspection/M_JMItemPreviewAlpha.M_JMItemPreviewAlpha"));
	UClass* Widget = LoadClass<UJMItemInspectionWidgetBase>(nullptr, TEXT("/ItemInspector/UI/WBP_JMItemInspection.WBP_JMItemInspection_C"));
	if (!TestNotNull(TEXT("Original key mesh"), Mesh) || !TestNotNull(TEXT("Original alpha material"), Material)
		|| !TestNotNull(TEXT("Original inspector WBP"), Widget)) return false;
	TestEqual(TEXT("UI material domain"), Material->MaterialDomain, MD_UI);
	UInputAction* InteractAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Balhwajeom/Input/IA_Interact.IA_Interact"));
	UInputMappingContext* InteractionContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Balhwajeom/Input/IMC_Interaction.IMC_Interaction"));
	TestNotNull(TEXT("IA_Interact"), InteractAction);
	TestNotNull(TEXT("IMC_Interaction"), InteractionContext);
	bool bHasFMapping = false;
	if (InteractionContext && InteractAction)
	{
		for (const FEnhancedActionKeyMapping& Mapping : InteractionContext->GetMappings())
		{
			bHasFMapping |= Mapping.Action == InteractAction && Mapping.Key == EKeys::F;
		}
	}
	TestTrue(TEXT("F key triggers IA_Interact"), bHasFMapping);
	const TCHAR* DataPath = TEXT("/Game/Balhwajeom/Tests/ItemInspection/DA_KeyInspection");
	const TCHAR* BPPath = TEXT("/Game/Balhwajeom/Tests/ItemInspection/BP_KeyInspection");
	auto SaveNew = [this](UPackage* Package, UObject* Asset)
	{
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
		return TestTrue(TEXT("Fixture saved"), UPackage::SavePackage(Package, Asset,
			*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args));
	};
	UJMItemInspectionData* Data = nullptr;
	if (FPackageName::DoesPackageExist(DataPath))
	{
		Data = LoadObject<UJMItemInspectionData>(nullptr, TEXT("/Game/Balhwajeom/Tests/ItemInspection/DA_KeyInspection.DA_KeyInspection"));
	}
	else
	{
		UPackage* Package = CreatePackage(DataPath);
		Data = NewObject<UJMItemInspectionData>(Package, TEXT("DA_KeyInspection"), RF_Public | RF_Standalone);
		Data->ItemId = TEXT("InspectionTestKey");
		Data->DisplayName = FText::FromString(TEXT("Old Key"));
		Data->DisplayCategory = FText::FromString(TEXT("Inspection fixture"));
		Data->Description = FText::FromString(TEXT("Drag inside the preview to rotate. Scroll to zoom. Escape returns the key to its world position."));
		Data->PreviewMesh = Mesh;
		Data->ViewSettings.InitialRotation = FRotator(0, 90, 0);
		if (!SaveNew(Package, Data)) return false;
	}
	if (!TestNotNull(TEXT("Test data"), Data)) return false;
	if (!FPackageName::DoesPackageExist(BPPath))
	{
		UPackage* Package = CreatePackage(BPPath);
		UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, TEXT("BP_KeyInspection"), BPTYPE_Normal,
			UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
		USCS_Node* MeshNode = BP->SimpleConstructionScript->CreateNode(UStaticMeshComponent::StaticClass(), TEXT("KeyMesh"));
		UStaticMeshComponent* MeshComponent = CastChecked<UStaticMeshComponent>(MeshNode->ComponentTemplate);
		MeshComponent->SetStaticMesh(Mesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		BP->SimpleConstructionScript->AddNode(MeshNode);
		USCS_Node* InspectionNode = BP->SimpleConstructionScript->CreateNode(UJMInspectableComponent::StaticClass(), TEXT("Inspectable"));
		UJMInspectableComponent* Inspectable = CastChecked<UJMInspectableComponent>(InspectionNode->ComponentTemplate);
		Inspectable->InspectionData = Data;
		Inspectable->bHideSourceActorDuringInspection = true;
		BP->SimpleConstructionScript->AddNode(InspectionNode);
		FKismetEditorUtilities::CompileBlueprint(BP);
		TestTrue(TEXT("Fixture blueprint compiles"), BP->Status != BS_Error);
		if (!SaveNew(Package, BP)) return false;
	}
	const TCHAR* MapPath = TEXT("/Game/Balhwajeom/Tests/ItemInspection/L_ItemInspectionTest");
	if (!FPackageName::DoesPackageExist(MapPath))
	{
		UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
		if (!TestNotNull(TEXT("Test world"), World)) return false;
		World->GetWorldSettings()->DefaultGameMode = ABalhwajeomCameraGameMode::StaticClass();
		World->SpawnActor<APlayerStart>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);

		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(150.0, 0.0, -55.0), FRotator::ZeroRotator);
		Floor->GetStaticMeshComponent()->SetStaticMesh(Cube);
		Floor->SetActorScale3D(FVector(12.0, 12.0, 1.0));
		Floor->SetActorLabel(TEXT("WalkableFloor"));

		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.0, 0.0, 400.0), FRotator(-45.0, -35.0, 0.0));
		Sun->GetLightComponent()->SetIntensity(8.0f);
		APointLight* Fill = World->SpawnActor<APointLight>(FVector(250.0, 100.0, 250.0), FRotator::ZeroRotator);
		Fill->GetLightComponent()->SetIntensity(2500.0f);
		CastChecked<UPointLightComponent>(Fill->GetLightComponent())->SetAttenuationRadius(1000.0f);

		UClass* KeyActorClass = LoadClass<AActor>(nullptr, TEXT("/Game/Balhwajeom/Tests/ItemInspection/BP_KeyInspection.BP_KeyInspection_C"));
		AActor* KeyActor = KeyActorClass
			? World->SpawnActor<AActor>(KeyActorClass, FVector(300.0, 100.0, 55.0), FRotator(0.0, 0.0, 90.0))
			: nullptr;
		if (!TestNotNull(TEXT("Inspectable key placed"), KeyActor)) return false;
		KeyActor->SetActorScale3D(FVector(4.0));
		KeyActor->SetActorLabel(TEXT("F_Key_Inspection_Test"));
		if (!TestTrue(TEXT("Test map saved"), UEditorLoadingAndSavingUtils::SaveMap(World, MapPath))) return false;
	}
	return true;
}
#endif
