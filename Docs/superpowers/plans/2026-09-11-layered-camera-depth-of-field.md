# Layered Camera Depth-of-Field Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the edge-preserving camera blur with a full-resolution near/far layered depth-of-field pipeline that blurs object silhouettes while preserving all existing evidence focus and capture rules.

**Architecture:** Keep the three serialized material paths and add two near-layer passes. Far color is blurred independently, near color is accumulated as premultiplied RGB plus coverage, and the final pass composites far first and near last using the existing signed sharp-region distances supplied by `UBalhwajeomPhotoCameraComponent`.

**Tech Stack:** Unreal Engine 5.7.4, C++17, Unreal Automation Tests, Unreal Python, post-process materials, User Scene Textures, DX12/SM6

**Spec:** `Docs/superpowers/specs/2026-09-11-layered-camera-depth-of-field-design.md`

## Global Constraints

- Do not create any further Git commits. The user explicitly prohibited commits after approving the spec.
- Preserve center-ray targeting, `CameraFocusPoint`, capture eligibility, focus-range offsets, FOV-independent distance rules, interpolation, and the 0.1-second focus grace period.
- A focused target uses its projected `CameraFocusPoint` depth plus/minus `BlurStartDistance`; evidence-table range offsets affect eligibility only.
- With no target, `[MinimumFocusDistance, MaximumFocusDistance]` remains the sharp region.
- Keep all passes after tonemapping, at full resolution, with built-in depth of field disabled and screen-space UI sharp.
- Preserve the existing three material paths and soft-object property names.
- Do not modify user-authored Blueprint, map, investigation-data, or individual model-material changes.
- Opaque and masked meshes must participate through scene depth without per-object setup. Translucent and separate-translucency content is documented as an exception in this iteration.
- Default `NearBlurRadiusScale` is `1.25`; default `FarBlurRadiusScale` is `1.0`.
- Retain `DepthRejectionDistance` only as a deprecated serialized property; the layered shader path must not use it.

---

### Task 1: Lock the Five-Pass Material Contract

**Files:**
- Modify: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`

**Interfaces:**
- Consumes: generated `UMaterial` assets and `UBalhwajeomPhotoCameraComponent` class defaults
- Produces: failing automation assertions for five ordered materials, four named full-resolution intermediates, alpha-capable near buffers, scalar parameters, and component defaults

- [x] **Step 1: Add expression/source inspection support.**

Add these includes and a helper that searches every Custom expression rather than relying only on asset names:

```cpp
#include "Materials/MaterialExpressionCustom.h"

bool CustomCodeContains(const UMaterial* Material, const TCHAR* Needle)
{
    TArray<const UMaterialExpressionCustom*> Expressions;
    Material->GetAllExpressionsInMaterialAndFunctionsOfType(Expressions);
    return Expressions.ContainsByPredicate(
        [Needle](const UMaterialExpressionCustom* Expression)
        {
            return Expression && Expression->Code.Contains(Needle);
        });
}
```

- [x] **Step 2: Replace the three-material expectations with the approved five-pass contract.**

Use these exact paths, priorities, and outputs:

```cpp
struct FExpectedPass
{
    const TCHAR* AssetPath;
    int32 Priority;
    FName OutputName;
};

const FExpectedPass ExpectedPasses[] = {
    { TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter.M_PP_CameraFocusPrefilter"), 0, TEXT("FocusFarHorizontal") },
    { TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.M_PP_CameraFocusBlur"), 1, TEXT("FocusFarBlurred") },
    { TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearHorizontal.M_PP_CameraFocusNearHorizontal"), 2, TEXT("FocusNearHorizontal") },
    { TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearVertical.M_PP_CameraFocusNearVertical"), 3, TEXT("FocusNearBlurred") },
    { TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite.M_PP_CameraFocusComposite"), 4, NAME_None }
};
```

For every intermediate pass assert `UserTextureDivisor == FIntPoint(1, 1)`. Require `BlendableOutputAlpha` on both near passes, `NearCoverage` in both near Custom expressions, `CenterDepth > SharpFarDistance` in both far passes, and `NearInput.a` in the composite. Require every User Scene Texture reader to be clamped.

- [x] **Step 3: Update scalar and component-default assertions.**

The material contract must expose:

```cpp
const TArray<FName> FocusParameters = {
    TEXT("SharpNearDistance"),
    TEXT("SharpFarDistance"),
    TEXT("BlurTransitionDistance"),
    TEXT("MaximumBlurStrength"),
    TEXT("MaximumBlurRadiusPixels"),
    TEXT("NearBlurRadiusScale"),
    TEXT("FarBlurRadiusScale")
};
```

Add soft-object default assertions for `FocusNearHorizontalMaterial` and `FocusNearVerticalMaterial`, and exact float-default assertions for both radius scales. Change the `DepthRejectionDistance` assertion to retain its serialized default of `100.0f`, require `DepthRejectionProperty->HasMetaData(TEXT("DeprecatedProperty"))`, and require a non-empty `DeprecationMessage`. This preserves the serialized property name while marking it deprecated in the editor.

- [x] **Step 4: Build and run the focused tests to prove the old implementation fails.**

Run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' BalhwajeomEditor Win64 Development 'C:\_UserProjects\Unreal\Balhwajeom_Project\Balhwajeom.uproject' -WaitMutex
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\_UserProjects\Unreal\Balhwajeom_Project\Balhwajeom.uproject' -unattended -nop4 -nosplash '-ExecCmds=Automation RunTests Balhwajeom.Camera.FocusBlur;Quit' '-TestExit=Automation Test Queue Empty'
```

Expected: compilation succeeds; the material/default tests fail because the two near assets and component properties do not exist and the old pass contract still uses priorities 0-2.

- [x] **Step 5: Record the checkpoint without committing.**

Run `git diff --check` and `git status --short`. Confirm only the intended test file and already-dirty user files are present.

---

### Task 2: Generate the Layered Full-Resolution Materials

**Files:**
- Modify: `Scripts/Camera/CreateCameraFocusBlurMaterial.py`
- Regenerate: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter.uasset`
- Regenerate: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.uasset`
- Create: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearHorizontal.uasset`
- Create: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearVertical.uasset`
- Regenerate: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite.uasset`

**Interfaces:**
- Consumes: `PostProcessInput0`, `SceneDepth`, sharp-region scalars, strength, radius, and layer scales
- Produces: `FocusFarHorizontal`, `FocusFarBlurred`, `FocusNearHorizontal`, `FocusNearBlurred`, and final scene color

- [x] **Step 1: Add exact asset constants and remove depth rejection from generated focus parameters.**

```python
FAR_HORIZONTAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusPrefilter"
FAR_VERTICAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusBlur"
NEAR_HORIZONTAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusNearHorizontal"
NEAR_VERTICAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusNearVertical"
COMPOSITE_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusComposite"

FOCUS_DEFAULTS = {
    "SharpNearDistance": 400.0,
    "SharpFarDistance": 1000.0,
    "BlurTransitionDistance": 500.0,
    "MaximumBlurStrength": 0.6,
    "MaximumBlurRadiusPixels": 12.0,
    "NearBlurRadiusScale": 1.25,
    "FarBlurRadiusScale": 1.0,
}
```

- [x] **Step 2: Generalize Custom expressions and material outputs for RGBA near buffers.**

Change `create_custom` to accept `output_type`. Add a `connect_float4_output` helper that connects RGB to `MP_EMISSIVE_COLOR`, enables `blendable_output_alpha`, masks A with `UMaterialExpressionComponentMask`, and connects it to `MP_OPACITY`:

```python
custom.set_editor_property("output_type", output_type)
material.set_editor_property("blendable_output_alpha", True)
alpha = unreal.MaterialEditingLibrary.create_material_expression(
    material, unreal.MaterialExpressionComponentMask, 220, 120)
alpha.set_editor_property("r", False)
alpha.set_editor_property("g", False)
alpha.set_editor_property("b", False)
alpha.set_editor_property("a", True)
unreal.MaterialEditingLibrary.connect_material_expressions(custom, "", alpha, "Input")
unreal.MaterialEditingLibrary.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
```

- [x] **Step 3: Implement far horizontal and vertical passes.**

Use the existing full-resolution viewport-to-texture conversion and a dense 17-sample Gaussian kernel. The initial five-sample prototype was rejected by rendered validation because a 32-pixel radius left up to 18 pixels between samples and produced repeated silhouettes:

```hlsl
float FarProgress = saturate((CenterDepth - SharpFarDistance) / max(BlurTransitionDistance, 0.001));
float FarAmount = saturate(MaximumBlurStrength) * FarProgress * FarProgress;
float RadiusPixels = max(MaximumBlurRadiusPixels * FarBlurRadiusScale, 0.0) * FarAmount * ResolutionScale;
for (int Index = -8; Index <= 8; ++Index)
{
    float NormalizedOffset = float(Index) / 8.0;
    float Weight = exp2(-4.0 * NormalizedOffset * NormalizedOffset);
    // Gather at RadiusPixels * NormalizedOffset and normalize accepted weights.
}
```

Return untouched source color when `CenterDepth <= SharpFarDistance`. For a far destination, gather only samples where `SampleDepth > SharpFarDistance`, normalize the Gaussian weights, and do not apply `DepthRejectionDistance`. Write `FocusFarHorizontal` at priority 0 and `FocusFarBlurred` at priority 1.

- [x] **Step 4: Implement near horizontal contribution gathering.**

Each destination gathers nearby source pixels. Only source pixels in front of `SharpNearDistance` contribute, and coverage is premultiplied into RGB:

```hlsl
float MaxRadiusPixels = max(MaximumBlurRadiusPixels * NearBlurRadiusScale, 0.0)
    * saturate(MaximumBlurStrength) * ResolutionScale;
float NearProgress = saturate((SharpNearDistance - SampleDepth) / max(BlurTransitionDistance, 0.001));
float NearCoverage = saturate(MaximumBlurStrength) * NearProgress * NearProgress;
float SourceRadius = max(MaximumBlurRadiusPixels * NearBlurRadiusScale, 0.0) * NearCoverage * ResolutionScale;
float NormalizedOffset = float(Index) / 8.0;
float SampleOffsetPixels = abs(NormalizedOffset) * MaxRadiusPixels;
float Support = saturate(SourceRadius - SampleOffsetPixels + 1.0);
float Weight = exp2(-4.0 * NormalizedOffset * NormalizedOffset);
float Contribution = Weight * NearCoverage * Support;
PremultipliedColor += SampleColor * Contribution;
CoverageSum += Contribution;
return float4(PremultipliedColor, saturate(CoverageSum));
```

Sample from a maximum-radius horizontal footprint so foreground contributions can reach destination pixels outside the original silhouette. Write RGBA to `FocusNearHorizontal` at priority 2.

- [x] **Step 5: Implement near vertical accumulation.**

Read filtered RGBA from `FocusNearHorizontal`, gather vertically at the maximum near radius, and preserve premultiplication:

```hlsl
float4 NearSample = SceneTextureLookup(NearUV, NearInput.ID, true);
float Contribution = Weight * NearSample.a;
PremultipliedColor += NearSample.rgb * Weight;
CoverageSum += Contribution;
return float4(
    PremultipliedColor / max(KernelWeightSum, 0.0001),
    saturate(CoverageSum / max(KernelWeightSum, 0.0001)));
```

Normalize premultiplied RGB and coverage by the same kernel weight sum. Write RGBA to `FocusNearBlurred` at priority 3 so the composite can unpremultiply the color exactly once. Clamp every User Scene Texture sample.

- [x] **Step 6: Implement the ordered final composite.**

```hlsl
float FarProgress = saturate((CenterDepth - SharpFarDistance) / max(BlurTransitionDistance, 0.001));
float FarAmount = saturate(MaximumBlurStrength) * FarProgress * FarProgress;
float3 BaseColor = lerp(SceneColorInput.rgb, FarInput.rgb, FarAmount);

float NearCoverage = saturate(NearInput.a);
float3 NearColor = NearInput.rgb / max(NearCoverage, 0.0001);
return lerp(BaseColor, NearColor, NearCoverage);
```

Read both `FocusFarBlurred` and `FocusNearBlurred`, preserve exact scene color throughout the sharp slab when near coverage is zero, and run at priority 4.

- [x] **Step 7: Regenerate all five assets with Unreal Python.**

Run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\_UserProjects\Unreal\Balhwajeom_Project\Balhwajeom.uproject' -run=pythonscript '-script=C:\_UserProjects\Unreal\Balhwajeom_Project\Scripts\Camera\CreateCameraFocusBlurMaterial.py' -unattended -nop4 -nosplash
```

Expected: all five materials compile and save; the log contains no `LogMaterial: Error`, missing User Scene Texture, or Custom expression compile error.

- [x] **Step 8: Run the material contract test.**

Run `Automation RunTests Balhwajeom.Camera.FocusBlurMaterial`. Expected: the material asset test passes. The component-default test may still fail until Task 3.

- [x] **Step 9: Record the checkpoint without committing.**

Run `git diff --check` and list the five material assets. Confirm no Blueprint, map, table, or surface-material asset was generated or saved by this task.

---

### Task 3: Bind Five Materials Atomically in the Photo Camera Component

**Files:**
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp`
- Modify: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`

**Interfaces:**
- Consumes: the five material assets and current `FBalhwajeomFocusRegion`
- Produces: five ordered dynamic material instances receiving a common scalar contract; two layer-specific radius scales; one-shot initialization failure behavior

- [x] **Step 1: Add new soft pointers, tunables, transient instances, and initialization state.**

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence Focus|Blur")
TSoftObjectPtr<UMaterialInterface> FocusNearHorizontalMaterial;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence Focus|Blur")
TSoftObjectPtr<UMaterialInterface> FocusNearVerticalMaterial;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0"))
float NearBlurRadiusScale = 1.25f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0"))
float FarBlurRadiusScale = 1.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy",
    meta = (ClampMin = "0.1", Units = "cm", DeprecatedProperty,
        DeprecationMessage = "The layered focus blur no longer preserves depth edges with bilateral rejection."))
float DepthRejectionDistance = 100.0f;

UPROPERTY(Transient)
TObjectPtr<UMaterialInstanceDynamic> FocusNearHorizontalMaterialInstance;

UPROPERTY(Transient)
TObjectPtr<UMaterialInstanceDynamic> FocusNearVerticalMaterialInstance;

bool bFocusBlurInitializationFailed = false;
```

- [x] **Step 2: Set exact constructor asset defaults.**

```cpp
FocusNearHorizontalMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
    TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearHorizontal.M_PP_CameraFocusNearHorizontal")));
FocusNearVerticalMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
    TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearVertical.M_PP_CameraFocusNearVertical")));
```

- [x] **Step 3: Make initialization all-or-nothing.**

Load all five bases before creating or adding any MID. If any load fails, set `bFocusBlurInitializationFailed`, emit exactly one warning naming all missing paths, and return. Create all five MIDs into local pointers; only assign member pointers and call `AddOrUpdateBlendable` after every creation succeeds. The ready condition is:

```cpp
const bool bAllReady = FocusPrefilterMaterialInstance && FocusBlurMaterialInstance &&
    FocusNearHorizontalMaterialInstance && FocusNearVerticalMaterialInstance &&
    FocusCompositeMaterialInstance;
```

- [x] **Step 4: Clear the complete chain when camera mode exits.**

Where the current three MIDs are nulled after restoring `SavedPhotoPostProcessSettings`, also clear both near MIDs and reset `bFocusBlurInitializationFailed = false` so a later camera session may retry after assets are repaired.

- [x] **Step 5: Send the common contract to all five MIDs.**

```cpp
MaterialInstance->SetScalarParameterValue(TEXT("SharpNearDistance"), CurrentFocusRegion.SharpNear);
MaterialInstance->SetScalarParameterValue(TEXT("SharpFarDistance"), CurrentFocusRegion.SharpFar);
MaterialInstance->SetScalarParameterValue(TEXT("BlurTransitionDistance"), FMath::Max(0.0f, BlurTransitionDistance));
MaterialInstance->SetScalarParameterValue(TEXT("MaximumBlurStrength"), CurrentMaximumBlurStrength);
MaterialInstance->SetScalarParameterValue(TEXT("MaximumBlurRadiusPixels"), FMath::Max(0.0f, MaximumBlurRadiusPixels));
MaterialInstance->SetScalarParameterValue(TEXT("NearBlurRadiusScale"), FMath::Max(0.0f, NearBlurRadiusScale));
MaterialInstance->SetScalarParameterValue(TEXT("FarBlurRadiusScale"), FMath::Max(0.0f, FarBlurRadiusScale));
```

Remove the runtime `DepthRejectionDistance` material assignment. Keep the existing built-in DOF override at zero.

- [x] **Step 6: Build and run the focused component tests.**

Run the Development Editor build and `Automation RunTests Balhwajeom.Camera.FocusBlur`. Expected: both material and component-default tests pass.

- [x] **Step 7: Record the checkpoint without committing.**

Run `git diff --check`, inspect the component diff, and confirm targeting/capture functions outside initialization, application, and teardown were not changed.

---

### Task 4: Validate Near Coverage and Optical Edge Behavior

**Files:**
- Modify if required by observed alpha loss: `Scripts/Camera/CreateCameraFocusBlurMaterial.py`
- Modify if required by observed alpha loss: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`
- Regenerate if required: the five camera material assets under `Content/Balhwajeom/Camera/Materials/`

**Interfaces:**
- Consumes: the complete five-pass pipeline in the prototype renderer
- Produces: evidence that RGBA near coverage survives User Scene Texture storage and that silhouettes blur in the intended direction

- [x] **Step 1: Inspect User Scene Texture allocation and near alpha in the prototype.**

Open `/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype`, enter camera mode, focus the rear curtain while a foreground primitive crosses it, and run:

```text
r.PostProcessing.UserSceneTextureDebug 1
```

Inspect `FocusNearHorizontal` and `FocusNearBlurred`. Expected: alpha/coverage is present over the near object and extends smoothly outside its original silhouette; it is zero for isolated sharp/far regions.

- [x] **Step 2: Exercise both focus directions.**

Capture comparison frames at 1280x720 and 1920x1080:

1. Rear curtain focused: the closer primitive's texture and outer contour both soften over the curtain.
2. Foreground evidence focused: its contour remains crisp while the curtain softens behind it.
3. No evidence centered: `[MinimumFocusDistance, MaximumFocusDistance]` stays readable while only its exterior blurs.

Reject the implementation if blur shifts relative to geometry, has horizontal/vertical streaking, creates black fringes, or leaks far color over a focused foreground contour.

- [x] **Step 3: Apply the defined fallback only if alpha is absent.**

If the debug view proves that `FocusNearHorizontal.a` or `FocusNearBlurred.a` is discarded on DX12/SM6, do not tune around it. Replace RGBA packing with separate full-resolution outputs named `FocusNearColorHorizontal`, `FocusNearCoverageHorizontal`, `FocusNearColorBlurred`, and `FocusNearCoverageBlurred`; add the corresponding coverage writer/reader material passes and update the asset test and component soft pointers atomically. Keep the same premultiplied equations and composite order. If alpha is present, do not add these extra passes.

- [ ] **Step 4: Verify ordinary and exceptional content behavior.**

Place or reuse an ordinary opaque mesh and confirm it blurs without tag, interface, component, or surface-material edits. Inspect one translucent object separately and record that its behavior is outside the opaque scene-depth guarantee; do not modify that content material in this task.

- [ ] **Step 5: Record performance.**

At 1920x1080, compare the camera post-process GPU timing before/after using `profilegpu` or Unreal Insights. Record the five-pass total and flag a regression for review rather than introducing an unapproved half-resolution mode.

---

### Task 5: Regression Verification and Handoff Documentation

**Files:**
- Modify: `Docs/Systems/Camera-Handoff.md`

**Interfaces:**
- Consumes: final layered blur behavior and measured limitations
- Produces: tuning documentation, verification evidence, and a clean uncommitted handoff

- [x] **Step 1: Document the final pipeline and tuning ownership.**

Add the exact five pass roles, the `NearBlurRadiusScale`/`FarBlurRadiusScale` defaults, the deprecated status of `DepthRejectionDistance`, automatic opaque/masked model participation, and the translucent/separate-translucency exception. State that autofocus/capture still requires evidence actor setup and `CameraFocusPoint`.

- [x] **Step 2: Run the Development Editor build.**

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' BalhwajeomEditor Win64 Development 'C:\_UserProjects\Unreal\Balhwajeom_Project\Balhwajeom.uproject' -WaitMutex
```

Expected: exit code 0 with no compile errors or new warnings in the modified camera files.

- [x] **Step 3: Run camera and interaction automation.**

Run `Automation RunTests Balhwajeom.Camera` and `Automation RunTests Balhwajeom.Interaction`. Expected: all tests pass.

- [x] **Step 4: Run investigation automation separately.**

Run `Automation RunTests Balhwajeom.Investigation`. Expected: no new failure caused by camera rendering. If `ConfiguredDataValidation` still reports the previously observed pillow story-cue count mismatch, identify it as an unrelated pre-existing data failure rather than modifying investigation data.

- [x] **Step 5: Inspect the final working tree without committing.**

Run:

```powershell
git diff --check
git status --short
```

Confirm the intended implementation consists only of the camera generator/material assets, photo-camera component, camera material tests, and camera handoff document. Preserve every unrelated pre-existing user change. Do not stage or commit any file.
