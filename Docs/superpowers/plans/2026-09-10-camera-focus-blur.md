# Camera Focus and Blur Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the approved fixed-range, center-ray photo focus system and depth-driven Ease-In blur.

**Architecture:** Keep deterministic range, blur, and grace calculations in a small runtime focus model that automation tests can exercise without a viewport. `UBalhwajeomPhotoCameraComponent` owns camera configuration and strict ray acquisition, `ABalhwajeomEvidenceActor`/`FEvidenceStateDefinition` supply per-target offsets, and a generated post-process material renders the exact depth curve.

**Tech Stack:** Unreal Engine 5.7, C++20 through Unreal Build Tool, Unreal Automation Tests, UMG, post-process materials.

**Spec:** `docs/superpowers/specs/2026-09-10-camera-focus-blur-design.md`

## Global Constraints

- FOV affects zoom presentation only and never changes focus distance or eligibility.
- Global default focus range is 400..1000 cm; per-state/legacy actor offsets default to zero.
- Effective minimum distances are clamped to zero; an effective minimum greater than its maximum rejects the target.
- Only the first `ECC_Visibility` hit through viewport center may become the strict target.
- Visual target grace is exactly 0.1 seconds by default; strict capture never uses grace.
- Remove the projected 70% coverage gate from capture.
- Preserve investigation `bCanCapture`, `PhotoID`, pending-save, and duplicate-photo validation.
- Preserve the existing focus-guide WBP and status-icon data flow.

---

### Task 1: Deterministic focus rules

**Files:**
- Create: `Source/Balhwajeom/CameraSystem/BalhwajeomCameraFocusModel.h`
- Create: `Source/Balhwajeom/CameraSystem/BalhwajeomCameraFocusModel.cpp`
- Create: `Source/Balhwajeom/Private/CameraSystem/Test/BalhwajeomCameraFocusModelTest.cpp`

**Interfaces:**
- Produces: `FBalhwajeomFocusRange`, `FBalhwajeomFocusVisualState`, `CalculateEffectiveRange`, `CalculateDesiredFocusRegion`, `CalculateBlurStrength`, and `UpdateRetainedTarget`.
- Consumes: scalar camera settings, object offsets, target identity, and elapsed time; no world or viewport state.

- [ ] **Step 1: Write failing automation tests**

Cover a 400..1000 global range, positive/negative offsets, zero clamping, invalid min/max rejection, unchanged results for different FOV values, focused and default sharp ranges, quadratic Ease-In at 0/0.5/1 progress, immediate target replacement, reacquisition inside 0.1 seconds, and expiration at 0.1 seconds.

- [ ] **Step 2: Build and run the focused model test to verify RED**

Run an Editor build. Expected: compilation fails because `BalhwajeomCameraFocusModel.h` and its API do not exist.

- [ ] **Step 3: Implement the minimal pure model**

Use non-negative range clamping without bound swapping, quadratic `Progress * Progress`, and a retention state containing current target, invalid elapsed time, and whether the target is strict or grace-retained.

- [ ] **Step 4: Build and run `Balhwajeom.Camera.FocusModel` to verify GREEN**

Expected: every deterministic model assertion passes.

### Task 2: Move focus-distance ownership to camera and offsets

**Files:**
- Modify: `Source/Balhwajeom/Public/Investigation/EvidenceDefinitions.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceTypes.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceActor.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomEvidenceActor.cpp`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp`
- Extend test: `Source/Balhwajeom/Private/CameraSystem/Test/BalhwajeomCameraFocusModelTest.cpp`

**Interfaces:**
- Consumes: `CalculateEffectiveRange` from Task 1 and current investigation state lookup.
- Produces: camera properties `MinimumFocusDistance`, `MaximumFocusDistance`, `BlurStartDistance`, `BlurTransitionDistance`, `MaximumBlurStrength`, `FocusApplicationSpeed`, and `FocusTargetGracePeriod`; target offsets `MinimumFocusDistanceOffset` and `MaximumFocusDistanceOffset`.

- [ ] **Step 1: Add failing source-of-truth tests**

Verify that resolved investigation targets return state offsets, legacy targets return actor offsets, and the zoom/FOV value cannot alter the effective range.

- [ ] **Step 2: Run the focused test to verify RED**

Expected: compilation fails because the offset fields and new resolved-target members do not exist.

- [ ] **Step 3: Add the new properties and compatibility migration**

Add zero-default offsets to the state and target-info structs and actor. Populate them in `ApplyInvestigationState` and `RequestCameraTargetInfo`; retain the old preferred/tolerance fields as deprecated serialized compatibility data.

- [ ] **Step 4: Run the model/source-of-truth test to verify GREEN**

Expected: investigation-state data wins, legacy actor offsets work, and FOV is absent from range calculation.

### Task 3: Center-ray-only acquisition and grace retention

**Files:**
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h`
- Modify: `Source/BalhwajeomEditor/Private/CameraSystem/Test/InvestigationPrototypeFocusTest.cpp`

**Interfaces:**
- Consumes: target info/offsets and retention helpers from Tasks 1–2.
- Produces: strict center target, grace-retained visual target, and a strict capture refresh path.

- [ ] **Step 1: Add failing integration assertions**

Exercise a centered target, an off-center visible target, a centered occluded target, a brief 0.05-second miss, a 0.11-second miss, and a shutter request during a retained-but-invalid visual focus.

- [ ] **Step 2: Run `Balhwajeom.Camera.PrototypeCenteredFocus` to verify RED**

Expected: the old silhouette search acquires off-center targets, has no explicit grace state, and still routes capture through projected coverage.

- [ ] **Step 3: Replace candidate enumeration with one strict center trace**

Resolve only the first center hit, validate its interface and effective focus-point range, update the retained visual state, and project `CameraFocusPoint` for the guide. Add a strict, no-grace refresh used by `TakePhoto`.

- [ ] **Step 4: Remove the projected coverage capture gate**

Delete `bActiveFocusTargetFramedEnough` from shutter eligibility while retaining any bounds helper still needed by unrelated display code. Do not remove state authorization or duplicate/pending checks.

- [ ] **Step 5: Run the focused integration tests to verify GREEN**

Expected: center/occlusion/range behavior matches the spec and a stale grace target cannot be captured.

### Task 4: Exact depth-driven Ease-In blur

**Files:**
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp`
- Create: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.uasset`
- Create: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`

**Interfaces:**
- Consumes: current/desired focus-region bounds and `CalculateBlurStrength` semantics.
- Produces: dynamic material parameters `SharpNearDistance`, `SharpFarDistance`, `BlurTransitionDistance`, and `MaximumBlurStrength`.

- [ ] **Step 1: Add a failing material asset test**

Require `/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur`, Post Process material domain, and all four scalar parameter names.

- [ ] **Step 2: Run `Balhwajeom.Camera.FocusBlurMaterial` to verify RED**

Expected: the material asset cannot be loaded.

- [ ] **Step 3: Generate the post-process material**

Build a nine-tap symmetric blur of `PostProcessInput0`. Compute outside distance from scene depth, normalize by transition distance, square the saturated progress, multiply sample radius by normalized maximum strength, and connect the result to emissive color.

- [ ] **Step 4: Apply and update a dynamic material instance**

Create one MID on camera-mode entry, append it to the camera weighted blendables, interpolate current focal distance and sharp bounds with `FocusApplicationSpeed`, update all scalar parameters every focus tick, disable the former F-stop override, and restore the saved post-process state on exit.

- [ ] **Step 5: Build and run model plus material tests to verify GREEN**

Expected: the asset contract passes and the runtime component compiles with exact parameter updates.

### Task 5: Full regression verification and handoff

**Files:**
- Modify if required by test fixtures: `Content/Balhwajeom/Maps/Prototype/L_InvestigationPrototype.umap`
- Update: `docs/Systems/Camera-Handoff.md`

**Interfaces:**
- Consumes: all previous tasks.
- Produces: verified prototype behavior and designer-facing ownership/tuning documentation.

- [ ] **Step 1: Run the full camera automation group**

Run `Automation RunTests Balhwajeom.Camera`. Expected: all focus model, material, asset, and prototype tests pass.

- [ ] **Step 2: Run investigation and interaction regressions**

Run `Automation RunTests Balhwajeom.Investigation` and the existing player interaction test group. Expected: no failures.

- [ ] **Step 3: Build Development Editor**

Run Unreal Build Tool for `BalhwajeomEditor Win64 Development`. Expected: exit code 0 with no compile or link errors.

- [ ] **Step 4: Update the camera handoff**

Document the player component settings, state-table offsets, `CameraFocusPoint` authoring, strict shutter behavior, 0.1-second visual grace, material parameters, and a short PIE tuning checklist.

- [ ] **Step 5: Inspect the final diff**

Confirm no unrelated assets or source files changed, no old FOV-scaled focus calculation remains active, and no projected-coverage check remains in shutter eligibility.

