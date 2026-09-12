#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionUserSceneTexture.h"
#include "UObject/UnrealType.h"

namespace
{
	UMaterial* LoadFocusMaterial(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const TCHAR* AssetPath,
		const int32 ExpectedPriority)
	{
		UMaterial* Material = LoadObject<UMaterial>(nullptr, AssetPath);
		Test.TestNotNull(Label, Material);
		if (Material)
		{
			Test.TestEqual(
				FString::Printf(TEXT("%s uses the post-process domain"), Label),
				Material->MaterialDomain.GetValue(),
				MD_PostProcess);
			Test.TestEqual(
				FString::Printf(TEXT("%s runs after tonemapping"), Label),
				Material->BlendableLocation.GetValue(),
				BL_SceneColorAfterTonemapping);
			Test.TestEqual(
				FString::Printf(TEXT("%s has deterministic pass priority"), Label),
				Material->BlendablePriority,
				ExpectedPriority);
		}
		return Material;
	}

	void TestScalarParameters(
		FAutomationTestBase& Test,
		UMaterial* Material,
		const TArray<FName>& ExpectedNames)
	{
		if (!Material)
		{
			return;
		}

		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		Material->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);
		for (const FName ParameterName : ExpectedNames)
		{
			Test.TestTrue(
				FString::Printf(TEXT("Material exposes scalar parameter %s"), *ParameterName.ToString()),
				ParameterInfos.ContainsByPredicate(
					[ParameterName](const FMaterialParameterInfo& Info)
					{
						return Info.Name == ParameterName;
					}));
		}
	}

	void TestUserSceneTexturesAreClamped(
		FAutomationTestBase& Test,
		UMaterial* Material)
	{
		if (!Material)
		{
			return;
		}

		TArray<const UMaterialExpressionUserSceneTexture*> UserSceneTextures;
		Material->GetAllExpressionsInMaterialAndFunctionsOfType(UserSceneTextures);
		Test.TestTrue(TEXT("Material reads at least one user scene texture"),
			!UserSceneTextures.IsEmpty());
		for (const UMaterialExpressionUserSceneTexture* UserSceneTexture : UserSceneTextures)
		{
			Test.TestTrue(
				FString::Printf(TEXT("User scene texture %s clamps samples to its valid viewport"),
					*UserSceneTexture->UserSceneTexture.ToString()),
				UserSceneTexture->bClamped);
		}
	}

	bool CustomCodeContains(const UMaterial* Material, const TCHAR* Needle)
	{
		if (!Material)
		{
			return false;
		}

		TArray<const UMaterialExpressionCustom*> Expressions;
		Material->GetAllExpressionsInMaterialAndFunctionsOfType(Expressions);
		return Expressions.ContainsByPredicate(
			[Needle](const UMaterialExpressionCustom* Expression)
			{
				return Expression && Expression->Code.Contains(Needle);
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomCameraFocusBlurMaterialTest,
	"Balhwajeom.Camera.FocusBlurMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomCameraFocusBlurMaterialTest::RunTest(const FString& Parameters)
{
	UMaterial* FarHorizontal = LoadFocusMaterial(
		*this,
		TEXT("Far horizontal focus blur material"),
		TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter.M_PP_CameraFocusPrefilter"),
		0);
	UMaterial* FarVertical = LoadFocusMaterial(
		*this,
		TEXT("Far vertical focus blur material"),
		TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.M_PP_CameraFocusBlur"),
		1);
	UMaterial* NearHorizontal = LoadFocusMaterial(
		*this,
		TEXT("Near horizontal focus blur material"),
		TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearHorizontal.M_PP_CameraFocusNearHorizontal"),
		2);
	UMaterial* NearVertical = LoadFocusMaterial(
		*this,
		TEXT("Near vertical focus blur material"),
		TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearVertical.M_PP_CameraFocusNearVertical"),
		3);
	UMaterial* Composite = LoadFocusMaterial(
		*this,
		TEXT("Focus composite material"),
		TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite.M_PP_CameraFocusComposite"),
		4);

	struct FExpectedIntermediate
	{
		UMaterial* Material;
		const TCHAR* Label;
		FName OutputName;
	};
	const FExpectedIntermediate ExpectedIntermediates[] = {
		{FarHorizontal, TEXT("Far horizontal"), TEXT("FocusFarHorizontal")},
		{FarVertical, TEXT("Far vertical"), TEXT("FocusFarBlurred")},
		{NearHorizontal, TEXT("Near horizontal"), TEXT("FocusNearHorizontal")},
		{NearVertical, TEXT("Near vertical"), TEXT("FocusNearBlurred")}
	};
	for (const FExpectedIntermediate& Expected : ExpectedIntermediates)
	{
		if (!Expected.Material)
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s writes the named intermediate"), Expected.Label),
			Expected.Material->UserSceneTexture, Expected.OutputName);
		TestEqual(FString::Printf(TEXT("%s is full width"), Expected.Label),
			Expected.Material->UserTextureDivisor.X, 1);
		TestEqual(FString::Printf(TEXT("%s is full height"), Expected.Label),
			Expected.Material->UserTextureDivisor.Y, 1);
	}
	if (NearHorizontal)
	{
		TestTrue(TEXT("Near horizontal preserves coverage in output alpha"),
			NearHorizontal->BlendableOutputAlpha);
		TestTrue(TEXT("Near horizontal shader produces near coverage"),
			CustomCodeContains(NearHorizontal, TEXT("NearCoverage")));
	}
	if (NearVertical)
	{
		TestTrue(TEXT("Near vertical preserves coverage in output alpha"),
			NearVertical->BlendableOutputAlpha);
		TestTrue(TEXT("Near vertical shader propagates near coverage"),
			CustomCodeContains(NearVertical, TEXT("NearCoverage")));
	}
	TestTrue(TEXT("Far horizontal only gathers far depth"),
		CustomCodeContains(FarHorizontal, TEXT("SampleDepth > SharpFarDistance")));
	TestTrue(TEXT("Far vertical only gathers far depth"),
		CustomCodeContains(FarVertical, TEXT("SampleDepth > SharpFarDistance")));
	TestTrue(TEXT("Composite consumes near alpha coverage"),
		CustomCodeContains(Composite, TEXT("NearInput.a")));
	const TCHAR* DenseKernelLoop = TEXT("for (int Index = -8; Index <= 8; ++Index)");
	TestTrue(TEXT("Far horizontal uses a dense 17-sample kernel"),
		CustomCodeContains(FarHorizontal, DenseKernelLoop));
	TestTrue(TEXT("Far vertical uses a dense 17-sample kernel"),
		CustomCodeContains(FarVertical, DenseKernelLoop));
	TestTrue(TEXT("Near horizontal uses a dense 17-sample kernel"),
		CustomCodeContains(NearHorizontal, DenseKernelLoop));
	TestTrue(TEXT("Near vertical uses a dense 17-sample kernel"),
		CustomCodeContains(NearVertical, DenseKernelLoop));

	const TArray<FName> FocusParameters = {
		TEXT("SharpNearDistance"),
		TEXT("SharpFarDistance"),
		TEXT("BlurTransitionDistance"),
		TEXT("MaximumBlurStrength"),
		TEXT("MaximumBlurRadiusPixels"),
		TEXT("NearBlurRadiusScale"),
		TEXT("FarBlurRadiusScale")
	};
	TestScalarParameters(*this, FarHorizontal, FocusParameters);
	TestScalarParameters(*this, FarVertical, FocusParameters);
	TestScalarParameters(*this, NearHorizontal, FocusParameters);
	TestScalarParameters(*this, NearVertical, FocusParameters);
	TestScalarParameters(*this, Composite, FocusParameters);
	TestUserSceneTexturesAreClamped(*this, FarVertical);
	TestUserSceneTexturesAreClamped(*this, NearVertical);
	TestUserSceneTexturesAreClamped(*this, Composite);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomCameraFocusBlurComponentDefaultsTest,
	"Balhwajeom.Camera.FocusBlurComponentDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomCameraFocusBlurComponentDefaultsTest::RunTest(const FString& Parameters)
{
	const UBalhwajeomPhotoCameraComponent* Defaults =
		GetDefault<UBalhwajeomPhotoCameraComponent>();
	if (!TestNotNull(TEXT("Photo camera component defaults exist"), Defaults))
	{
		return false;
	}

	struct FExpectedMaterial
	{
		FName PropertyName;
		FString AssetPath;
	};
	const FExpectedMaterial ExpectedMaterials[] = {
		{ TEXT("FocusPrefilterMaterial"), TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter.M_PP_CameraFocusPrefilter") },
		{ TEXT("FocusBlurMaterial"), TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.M_PP_CameraFocusBlur") },
		{ TEXT("FocusNearHorizontalMaterial"), TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearHorizontal.M_PP_CameraFocusNearHorizontal") },
		{ TEXT("FocusNearVerticalMaterial"), TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearVertical.M_PP_CameraFocusNearVertical") },
		{ TEXT("FocusCompositeMaterial"), TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite.M_PP_CameraFocusComposite") }
	};
	for (const FExpectedMaterial& Expected : ExpectedMaterials)
	{
		const FSoftObjectProperty* Property = CastField<FSoftObjectProperty>(
			Defaults->GetClass()->FindPropertyByName(Expected.PropertyName));
		if (!TestNotNull(
			*FString::Printf(TEXT("Component exposes %s"), *Expected.PropertyName.ToString()),
			Property))
		{
			continue;
		}
		const FSoftObjectPtr& Value = Property->GetPropertyValue_InContainer(Defaults);
		TestEqual(
			*FString::Printf(TEXT("%s uses the expected asset"), *Expected.PropertyName.ToString()),
			Value.ToSoftObjectPath().GetAssetPathString(),
			Expected.AssetPath);
	}

	const FFloatProperty* RadiusProperty = CastField<FFloatProperty>(
		Defaults->GetClass()->FindPropertyByName(TEXT("MaximumBlurRadiusPixels")));
	if (TestNotNull(TEXT("Component exposes MaximumBlurRadiusPixels"), RadiusProperty))
	{
		TestTrue(TEXT("Maximum blur radius is positive"),
			RadiusProperty->GetPropertyValue_InContainer(Defaults) > 0.0f);
	}

	const FFloatProperty* NearScaleProperty = CastField<FFloatProperty>(
		Defaults->GetClass()->FindPropertyByName(TEXT("NearBlurRadiusScale")));
	if (TestNotNull(TEXT("Component exposes NearBlurRadiusScale"), NearScaleProperty))
	{
		TestEqual(TEXT("Near blur radius scale defaults to 1.25"),
			NearScaleProperty->GetPropertyValue_InContainer(Defaults), 1.25f);
	}

	const FFloatProperty* FarScaleProperty = CastField<FFloatProperty>(
		Defaults->GetClass()->FindPropertyByName(TEXT("FarBlurRadiusScale")));
	if (TestNotNull(TEXT("Component exposes FarBlurRadiusScale"), FarScaleProperty))
	{
		TestEqual(TEXT("Far blur radius scale defaults to 1.0"),
			FarScaleProperty->GetPropertyValue_InContainer(Defaults), 1.0f);
	}

	const FFloatProperty* DepthRejectionProperty = CastField<FFloatProperty>(
		Defaults->GetClass()->FindPropertyByName(TEXT("DepthRejectionDistance")));
	if (TestNotNull(TEXT("Component exposes DepthRejectionDistance"), DepthRejectionProperty))
	{
		TestEqual(TEXT("Deprecated depth rejection distance retains its serialized default"),
			DepthRejectionProperty->GetPropertyValue_InContainer(Defaults), 100.0f);
		TestTrue(TEXT("Depth rejection distance has deprecated-property metadata"),
			DepthRejectionProperty->HasMetaData(TEXT("DeprecatedProperty")));
		TestFalse(TEXT("Depth rejection distance explains its replacement status"),
			DepthRejectionProperty->GetMetaData(TEXT("DeprecationMessage")).IsEmpty());
	}
	return true;
}

#endif
