# Layered Camera Depth-of-Field Design

## Goal

Replace the current edge-preserving surface blur with a full-resolution, layered depth-of-field pipeline. The result must blur both textures and silhouettes like an optical camera while preserving the existing evidence targeting, capture eligibility, `CameraFocusPoint`, focus interpolation, and no-target exploration-range rules.

## Approved behavior

- When the center-ray focus system has a valid evidence target, the sharp depth slab is centered on that target's projected `CameraFocusPoint` depth and extends by `BlurStartDistance` on both sides. The evidence/table minimum and maximum focus offsets continue to affect target eligibility only; they do not change the visual sharp-slab width.
- Geometry in front of the sharp slab receives foreground blur. Geometry behind the slab receives background blur.
- Foreground blur spreads beyond the foreground silhouette onto pixels behind it, matching the appearance of an out-of-focus object close to a physical lens.
- A focused foreground object remains sharp while the background is blurred behind its silhouette.
- When there is no valid focus target, `[MinimumFocusDistance, MaximumFocusDistance]` remains sharp and only geometry outside that exploration range is blurred.
- `BlurTransitionDistance` retains the existing quadratic ease from no blur to the configured maximum.
- Focus values continue to interpolate at `FocusApplicationSpeed`, including the existing 0.1-second target-loss grace behavior.
- Camera UI and focus-guide widgets remain sharp because they are rendered outside the scene post-process.
- Ordinary opaque or masked meshes placed in the map participate automatically through scene color and scene depth. Their mesh materials require no changes and do not receive any camera blur material directly.
- Becoming an autofocus or capture target remains a separate concern: the actor must still satisfy the existing evidence-target rules and provide a valid `CameraFocusPoint` and evidence data.

## Problem in the current pipeline

The current two-pass blur divides pixels into near, sharp, and far classes, then rejects every sample whose class differs from the center pixel. It additionally reduces weights across depth changes with `DepthRejectionDistance`. The final composite applies blur using only the current pixel's depth classification.

That is an edge-preserving surface blur. It intentionally prevents foreground color from reaching adjacent background pixels, so textures soften but silhouettes stay unnaturally crisp. Raising the radius or `DepthRejectionDistance` cannot create correct optical defocus; removing rejection without separating layers would instead produce two-way color contamination and visible halos.

## Rendering architecture

The pipeline remains a full-resolution, after-tonemapping post-process. It uses two independent blur layers and a final ordered composite:

```text
PostProcessInput0 + SceneDepth
        |-- Far Horizontal --> Far Vertical --> FarBlurred
        |-- Near Horizontal -> Near Vertical -> NearBlurred + NearCoverage
        `--------------------------------------> Final Composite
```

The existing material asset paths remain in place for serialized-reference compatibility:

1. `M_PP_CameraFocusPrefilter` becomes the far-layer horizontal pass at priority 0.
2. `M_PP_CameraFocusBlur` becomes the far-layer vertical pass at priority 1.
3. `M_PP_CameraFocusComposite` remains the final composite pass at priority 4.

Two new assets are added:

4. `M_PP_CameraFocusNearHorizontal` creates the horizontal near color and coverage contribution at priority 2.
5. `M_PP_CameraFocusNearVertical` completes the vertical near color and coverage blur at priority 3.

All User Scene Textures use a `1x1` divisor. Every pass starts from viewport UV, converts coordinates independently for each sampled scene or user texture, and clamps the resulting texture UV. This preserves alignment across viewport sizes and editor view rectangles.

## Signed circle-of-confusion model

For scene depth `D`, sharp limits `SharpNearDistance` and `SharpFarDistance`, and transition distance `T`:

- `D < SharpNearDistance`: near signed CoC.
- `SharpNearDistance <= D <= SharpFarDistance`: zero CoC and exact scene color.
- `D > SharpFarDistance`: far signed CoC.

Near and far magnitudes use the existing quadratic response:

```text
OutsideDistance = distance from D to the nearest sharp-slab boundary
Progress        = saturate(OutsideDistance / max(T, epsilon))
BlurAmount      = saturate(MaximumBlurStrength) * Progress^2
RadiusPixels    = MaximumBlurRadiusPixels * BlurAmount * layer scale * resolution scale
```

The current 1080p-relative radius scaling is retained so perceived blur remains comparable between 1280x720 and 1920x1080.

## Far layer

The far layer is receiver-driven:

- Far pixels collect color only from far samples.
- Sharp and near samples never bleed into the far layer.
- Far samples may mix across different far depths. The current strong bilateral `DepthRejectionDistance` term is removed from the normal path so background silhouettes can soften instead of being preserved.
- Surviving weights are normalized to prevent darkening at boundaries.

The far-horizontal pass writes `FocusFarHorizontal`. The far-vertical pass reads it and writes `FocusFarBlurred`.

## Near layer and silhouette spread

The near layer is contribution-driven rather than current-pixel-mask driven. Because a post-process shader cannot scatter writes, each destination pixel implements this as a gather over nearby source pixels:

- Only near-depth source pixels generate near color and coverage contributions.
- Each gathered near source is accepted and weighted according to that source pixel's own CoC radius and its distance from the destination pixel.
- The horizontal and vertical passes blur premultiplied near color and near coverage together.
- The near layer is normalized from premultiplied color using coverage-safe division.
- Near coverage may be non-zero on a background or sharp pixel adjacent to a near silhouette. This is the mechanism that creates optical foreground bleed.

The preferred representation is one full-resolution RGBA User Scene Texture: RGB stores premultiplied near color and A stores near coverage. Before adopting it permanently, the implementation must render-validate that the selected User Scene Texture format preserves alpha on the project's DX12/SM6 renderer. If alpha is not preserved, the implementation must use separate full-resolution near-color and coverage outputs rather than encoding coverage into an unreliable channel.

The near-horizontal pass writes `FocusNearHorizontal`. The near-vertical pass writes `FocusNearBlurred` or, in the fallback representation, paired near-color and near-coverage outputs.

## Composite order

The final pass performs ordered compositing:

1. Start with untouched `PostProcessInput0`.
2. Replace or blend far pixels with `FocusFarBlurred` according to their far CoC amount.
3. Keep the sharp slab exact. Near objects also remain absent from the far layer, preventing background color from washing into a focused foreground edge.
4. Composite the normalized near color over the result using blurred near coverage and the configured near blur strength.

Near coverage is applied last because near geometry optically occludes the focused or far scene behind it. This ordering prevents the two-way haloing that would result from loosening the current depth rejection in one shared blur buffer.

## Runtime integration

`UBalhwajeomPhotoCameraComponent` continues to own focus-region selection and parameter interpolation. It initializes five dynamic material instances in deterministic blendable priority order and sends the same focus values to every relevant pass.

Existing parameters remain:

- `SharpNearDistance`
- `SharpFarDistance`
- `BlurTransitionDistance`
- `MaximumBlurStrength`
- `MaximumBlurRadiusPixels`
- `FocusApplicationSpeed`
- `bEnableEvidenceFocusBlur`

New parameters:

- `NearBlurRadiusScale`, default `1.25`
- `FarBlurRadiusScale`, default `1.0`

`DepthRejectionDistance` remains as a deprecated serialized property for Blueprint compatibility but is not used by the layered blur. Existing Blueprint instances therefore load without losing unrelated settings.

The built-in Unreal depth of field remains disabled while this custom pipeline is active. Target acquisition, focus-distance resolution, capture eligibility, `CameraFocusPoint` selection, and evidence-table lookup are unchanged.

## Asset and actor behavior

The camera materials are post-process assets, not surface materials. Adding an ordinary opaque or masked Static Mesh or Skeletal Mesh to a level automatically includes it in scene-depth blur. No per-model material instance, tag, interface, or component is needed solely for blur.

Autofocus and capture targeting are not automatic for arbitrary meshes. A mesh must be hosted by or associated with the existing evidence actor flow, have its evidence identifiers configured, and expose a valid `CameraFocusPoint` according to the current targeting contract.

Translucent materials, particles, separate translucency, and other render paths that do not provide normal opaque scene depth are explicit exceptions. They are not promised to match opaque silhouette blur in this iteration. The implementation must document any observed exception rather than modifying unrelated content materials.

## Failure handling and compatibility

- If any required blur material cannot load or create a dynamic instance, the component disables the complete custom blur chain for that camera session and emits one actionable warning. It must not leave a partially composited frame.
- Existing soft-object fields and the three existing material paths are preserved.
- No changes are made to evidence Blueprints, `BP_OrbitViewCharacter_Legacy`, the prototype map, investigation tables, or individual model materials as part of this rendering change.
- The pipeline stays after tonemapping, preserving the current LDR behavior and sharp screen-space UI.

## Performance constraints

- All passes remain full resolution to preserve alignment and avoid the previously observed low-resolution artifacts.
- Each horizontal or vertical pass uses a five-fetch bilinear-optimized Gaussian kernel rather than nine independent fetches where Unreal's filtered User Scene Texture sampling supports it.
- The initial implementation uses four blur passes plus one composite pass. No quality-level system or half-resolution mode is introduced until GPU measurements show it is required.
- The 1920x1080 camera view is the primary performance measurement. GPU cost is recorded before and after the change so visual quality is not accepted without a known cost.

## Verification

### Automated tests

- Material asset tests verify five material assets, full-resolution User Scene Texture divisors, deterministic pass priorities, clamped inputs, expected intermediate names, and required scalar parameters.
- Shader-source tests verify separate near and far paths, premultiplied near coverage, ordered composite logic, and absence of the old cross-class plus bilateral edge-preservation path.
- Component tests verify all five dynamic instances receive identical sharp-region and transition parameters plus the correct near/far radius scales.
- Existing `Balhwajeom.Camera`, `Balhwajeom.Interaction`, and `Balhwajeom.Investigation` tests are rerun. Known unrelated failures must be identified separately rather than attributed to this change.

### Render validation in `L_InvestigationPrototype`

1. Focus the background with a near object crossing it. The near object's texture and silhouette must blur and spread smoothly over the focused background.
2. Focus a foreground evidence object. Its edge must remain crisp while the background is blurred without washing into that edge.
3. Look at no valid evidence target. The full `[MinimumFocusDistance, MaximumFocusDistance]` exploration slab must remain sharp and only its exterior must blur.
4. Repeat at 1280x720 and 1920x1080. Blur placement must remain aligned, isotropic, and proportional.
5. Verify `WBP_EvidenceFocusGuide` and other screen-space camera UI remain sharp and correctly positioned.
6. Place an ordinary opaque test mesh with an ordinary surface material. Verify it participates in blur without any per-object setup.
7. Inspect a translucent test material and record its behavior as a documented render-path exception.
8. Enable a near-coverage debug view to confirm that coverage extends past near silhouettes without leaking from sharp or far sources.
9. Record 1920x1080 GPU timing for the five-pass camera post-process.

## Acceptance criteria

- Foreground and background defocus visibly soften silhouettes, not only interior texture detail.
- A focused foreground object remains clean against a blurred background.
- The existing focus target, capture rules, no-target range, interpolation, and UI behavior remain unchanged.
- Ordinary opaque/masked models require no blur-specific setup.
- There are no viewport-relative offsets, directional streaks, black fringes, or obvious bidirectional color halos at tested resolutions.
- The render cost and translucent-content limitations are measured and documented.
