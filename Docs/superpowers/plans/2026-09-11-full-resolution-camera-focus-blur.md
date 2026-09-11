# Full-Resolution Camera Focus Blur Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the misaligned half-resolution blur chain with an aligned full-resolution depth-aware separable blur while preserving the approved camera focus and capture rules.

**Architecture:** Keep the three existing material paths for asset compatibility. Repurpose the first two passes as full-resolution horizontal and vertical bilateral Gaussian passes, derive every color/depth sample from a shared viewport coordinate, and composite with a full-resolution depth mask.

**Tech Stack:** Unreal Engine 5.7, C++, Automation Tests, Unreal Python, post-process materials, User Scene Textures

**Spec:** `Docs/superpowers/specs/2026-09-11-full-resolution-camera-focus-blur-design.md`

## Global Constraints

- Preserve all center-ray focus, `CameraFocusPoint`, capture-range, grace-period, and FOV behavior.
- Preserve the existing three material asset paths and component soft-pointer property names.
- Do not modify user-authored Blueprint, map, or investigation-data changes.
- Keep the materials after tonemapping and the UI unblurred.
- Do not create commits in the shared dirty checkout.

---

### Task 1: Lock the Full-Resolution Material Contract

**Files:**
- Modify: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`

**Interfaces:**
- Consumes: the three generated material assets
- Produces: automation assertions for full-resolution divisors, intermediate names, scalar parameters, and clamped User Scene Texture inputs

- [x] **Step 1: Change the expected first output from `FocusPrefilter` to `FocusBlurHorizontal` and both intermediate divisors from 2 to 1.**
- [x] **Step 2: Require `DepthRejectionDistance` on both blur passes and require every User Scene Texture input to be clamped.**
- [x] **Step 3: Build and run `Balhwajeom.Camera.FocusBlurMaterial`; confirm it fails on the old half-resolution assets.**

### Task 2: Generate Aligned Full-Resolution Blur Passes

**Files:**
- Modify: `Scripts/Camera/CreateCameraFocusBlurMaterial.py`
- Regenerate: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter.uasset`
- Regenerate: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.uasset`
- Regenerate: `Content/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite.uasset`

**Interfaces:**
- Consumes: `PostProcessInput0`, `SceneDepth`, focus scalar parameters
- Produces: `FocusBlurHorizontal`, `FocusBlurred`, and final scene color

- [x] **Step 1: Replace the four-tap half-resolution prefilter with a full-resolution horizontal Gaussian pass.**
- [x] **Step 2: Make every sample start from `GetViewportUV(Parameters)`, add an offset based on `GetSceneTextureViewSize(Source.ID).zw`, and convert/clamp independently for source color and scene depth.**
- [x] **Step 3: Reject samples across near/sharp/far classes and apply the independent `DepthRejectionDistance` bilateral weight within a class.**
- [x] **Step 4: Convert the second pass to the matching full-resolution vertical blur and keep the third pass as an exact full-resolution composite.**
- [x] **Step 5: Run the Unreal Python generator and confirm all three materials compile and save without shader errors.**
- [x] **Step 6: Rerun `Balhwajeom.Camera.FocusBlurMaterial`; confirm the material contract passes.**

### Task 3: Bind the Independent Depth-Rejection Parameter

**Files:**
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.h`
- Modify: `Source/Balhwajeom/CameraSystem/BalhwajeomPhotoCameraComponent.cpp`
- Modify: `Source/BalhwajeomEditor/Private/CameraSystem/Test/CameraFocusBlurMaterialTest.cpp`

**Interfaces:**
- Consumes: `DepthRejectionDistance` from the player photo-camera component
- Produces: the scalar value on the horizontal and vertical blur MIDs every focus update

- [x] **Step 1: Add a failing component-default assertion for a positive `DepthRejectionDistance`.**
- [x] **Step 2: Add `DepthRejectionDistance = 100cm` under `Camera|Evidence Focus|Blur`.**
- [x] **Step 3: Send the common focus parameters to the first, second, and composite material instances.**
- [x] **Step 4: Build and run all `Balhwajeom.Camera` tests.**

### Task 4: Render and Regression Verification

**Files:**
- Modify: `Docs/Systems/Camera-Handoff.md`

**Interfaces:**
- Consumes: the completed camera blur pipeline
- Produces: documented tuning behavior and verified rendered output

- [x] **Step 1: Document the full-resolution pass roles and `DepthRejectionDistance`.**
- [x] **Step 2: Run a Development Editor build.**
- [x] **Step 3: Run camera, interaction, and investigation automation suites; report unrelated pre-existing failures separately.**
- [x] **Step 4: Render the prototype at 1280x720 and 1920x1080 and inspect focus alignment, edge bleeding, isotropy, and UI sharpness.**
- [x] **Step 5: Run `git diff --check` and confirm Blueprint, map, and data files were not changed by this implementation.**
