# Depth-Aware Camera Focus Blur Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the sparse 9-sample camera blur with a smooth, depth-aware, half-resolution multi-pass blur while preserving the approved center-target and capture rules.

**Architecture:** `UBalhwajeomPhotoCameraComponent` keeps strict center-ray acquisition and radial capture eligibility, but calculates a separate view-axis focus depth for rendering. Three ordered post-process materials produce a half-resolution prefilter, a separable depth-aware Gaussian blur, and a full-resolution depth-mask composite using UE 5.7 User Scene Textures.

**Tech Stack:** Unreal Engine 5.7 C++, Automation Tests, Unreal Python material generation, post-process materials, User Scene Texture intermediates

**Spec:** `C:/Users/User/Downloads/camera_focus_blur_spec_v2.md`

## Global Constraints

- Screen-center `Visibility` trace and first visible target remain authoritative.
- Capture eligibility uses the true camera-to-`CameraFocusPoint` distance and per-state min/max offsets.
- Visual focus uses camera-forward view depth so it matches `SceneDepth`.
- With a target, the sharp range is target depth ± `BlurStartDistance`.
- Without a target, `[MinimumFocusDistance, MaximumFocusDistance]` is sharp.
- Blur outside the sharp range reaches `MaximumBlurStrength` over `BlurTransitionDistance` with quadratic Ease In.
- FOV affects presentation only, never focus or capture range.
- The 0.1-second grace period remains visual-only; shutter validation remains fresh and strict.
- Existing WBP assets and capture/data-table behavior are not changed.

---

### Task 1: Separate Capture Distance From Visual Focus Depth

**Files:**
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomCameraFocusModel.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomCameraFocusModel.cpp`
- Modify: `Source/Balhwajeom/Private/CameraSystem/Test/BalhwajeomCameraFocusModelTest.cpp`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp`

**Interfaces:**
- Consumes: camera world location, normalized camera-forward vector, `CameraFocusPoint` world location
- Produces: `FBalhwajeomCameraFocusModel::CalculateVisualFocusDepth(const FVector&, const FVector&, const FVector&) -> float`

- [x] **Step 1: Write the failing visual-depth test**

```cpp
TestEqual(TEXT("Visual focus depth follows the camera forward axis"),
    FBalhwajeomCameraFocusModel::CalculateVisualFocusDepth(
        FVector::ZeroVector, FVector::ForwardVector, FVector(700.0f, 300.0f, 0.0f)),
    700.0f);
```

- [x] **Step 2: Build and run `Balhwajeom.Camera.FocusModel`**

Expected: compilation fails because `CalculateVisualFocusDepth` does not exist.

- [x] **Step 3: Implement the view-axis calculation**

```cpp
return FMath::Max(0.0f, FVector::DotProduct(
    FocusLocation - CameraLocation,
    CameraForward.GetSafeNormal()));
```

- [x] **Step 4: Use radial distance only in `FindStrictFocusTarget`, and view depth only when calculating `DesiredRegion`**

```cpp
const float VisualFocusDepth = FBalhwajeomCameraFocusModel::CalculateVisualFocusDepth(
    CameraLocation, CameraForward, FocusLocation);
DesiredRegion = FBalhwajeomCameraFocusModel::CalculateFocusedRegion(
    VisualFocusDepth, BlurStartDistance);
```

- [x] **Step 5: Build and rerun `Balhwajeom.Camera.FocusModel`**

Expected: all focus-model tests pass.

### Task 2: Define the Multi-Pass Material Contract

**Files:**
- Modify: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`

**Interfaces:**
- Consumes: generated material assets
- Produces: an automation contract for three material paths, intermediate texture names, half-resolution divisors, and shared scalar parameters

- [x] **Step 1: Replace the single-material assertion with three-pass assertions**

```cpp
Prefilter: /Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter
Blur:      /Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur
Composite: /Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite
```

The test requires `FocusPrefilter` and `FocusBlurred` User Scene Texture outputs, a `(2,2)` divisor for both intermediate passes, and parameters `SharpNearDistance`, `SharpFarDistance`, `BlurTransitionDistance`, `MaximumBlurStrength`, and `MaximumBlurRadiusPixels` on the passes that consume them.

- [x] **Step 2: Build and run `Balhwajeom.Camera.FocusBlurMaterial`**

Expected: failure because the prefilter/composite assets and User Scene Texture outputs do not exist.

### Task 3: Generate Smooth Depth-Aware Blur Materials

**Files:**
- Replace: `Scripts/Camera/CreateCameraFocusBlurMaterial.py`
- Create: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter.uasset`
- Replace: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.uasset`
- Create: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite.uasset`

**Interfaces:**
- Consumes: `PostProcessInput0`, `SceneDepth`, focus scalar parameters
- Produces: `FocusPrefilter` and `FocusBlurred` half-resolution User Scene Textures plus a full-resolution composite

- [x] **Step 1: Generate the prefilter pass**

Use four bilinear taps around the source pixel, write their weighted average to `FocusPrefilter`, set `UserTextureDivisor=(2,2)`, disable pre-exposure scaling, and run after tonemapping so camera HUD remains unaffected.

- [x] **Step 2: Generate the depth-aware separable blur pass**

The pass reads `FocusPrefilter`, calculates per-pixel quadratic blur amount from `SceneDepth`, and applies the horizontal half of a normalized 9-weight Gaussian kernel. Every sample weight is reduced when its depth differs from the center depth or lies on the opposite side of the sharp slab. The maximum radius is `MaximumBlurRadiusPixels * MaximumBlurStrength`, resolution-scaled from 1080p.

- [x] **Step 3: Generate the composite pass**

At full resolution, apply the vertical half of the depth-aware Gaussian kernel to `FocusBlurred`, recalculate the exact quadratic blur amount from full-resolution `SceneDepth`, and return:

```hlsl
lerp(PostProcessInput0.rgb, FocusBlurred.rgb, BlurAmount)
```

The full-resolution mask keeps the sharp region exact even though the blur source is half resolution.

- [x] **Step 4: Run UnrealEditor-Cmd with the generator**

```powershell
UnrealEditor-Cmd.exe Balhwajeom.uproject -run=pythonscript -script=Scripts/Camera/CreateCameraFocusBlurMaterial.py -unattended -nop4
```

Expected: all three material assets compile and save without shader errors.

- [x] **Step 5: Run `Balhwajeom.Camera.FocusBlurMaterial`**

Expected: material contract test passes.

### Task 4: Bind and Order the Three Passes

**Files:**
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp`
- Modify: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`

**Interfaces:**
- Consumes: three generated base materials
- Produces: three dynamic material instances added to the photo camera in prefilter → blur → composite order

- [x] **Step 1: Add a failing component-default test for all three material references and a positive blur radius**

Expected: failure because the component owns only one soft material pointer.

- [x] **Step 2: Replace the single pointer/MID with prefilter, blur, and composite pointers/MIDs**

```cpp
TSoftObjectPtr<UMaterialInterface> FocusPrefilterMaterial;
TSoftObjectPtr<UMaterialInterface> FocusBlurMaterial;
TSoftObjectPtr<UMaterialInterface> FocusCompositeMaterial;
float MaximumBlurRadiusPixels = 12.0f;
```

- [x] **Step 3: Initialize all MIDs once when camera mode begins and add them in deterministic order**

Set shared scalar parameters on the blur and composite MIDs each focus tick. Restore saved post-process settings and clear all three MIDs when camera mode exits.

- [x] **Step 4: Build and run camera tests**

Expected: `Balhwajeom.Camera.*` passes with zero failures.

### Task 5: Regression and Visual Verification

**Files:**
- Modify: `Docs/Systems/Camera-Handoff.md`

**Interfaces:**
- Consumes: completed focus logic and material pipeline
- Produces: verified runtime behavior and current tuning documentation

- [x] **Step 1: Update the handoff document**

Document radial eligibility vs view-depth rendering, the three material assets, `MaximumBlurRadiusPixels`, and the fact that UI is composited after the scene blur.

- [x] **Step 2: Run the Development Editor build**

Expected: build exits with code 0.

- [x] **Step 3: Run focused automation suites**

Run `Balhwajeom.Camera.*`, `Balhwajeom.Investigation.*`, and interaction regression tests. Record any unrelated pre-existing failure separately.

- [x] **Step 4: Open `L_InvestigationPrototype` and visually check the reported scene**

Acceptance criteria:

- The large world-space prototype text becomes a continuous soft blur, never multiple offset copies.
- The focused `CameraFocusPoint` slab is sharp.
- Near and far geometry transition smoothly without bright/black edge halos.
- With no center target, 400–1000 cm remains sharp and only the outside regions blur.
- FOV changes do not alter focus eligibility or the computed focus depth.
- Camera guide WBP and frame remain sharp.

- [x] **Step 5: Inspect the final diff and preserve unrelated user changes**

Expected: only focus logic, focus materials/tests, generator, and camera documentation are changed by this task.
