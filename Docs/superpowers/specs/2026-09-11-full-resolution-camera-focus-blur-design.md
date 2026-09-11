# Full-Resolution Camera Focus Blur Design

## Goal

Make the custom photo-camera blur align with the scene geometry at every supported viewport size and remove the low-resolution, directional streaking visible around depth discontinuities.

## Approved behavior

- The existing center-ray target acquisition, `CameraFocusPoint` focus depth, capture eligibility, grace period, and focus interpolation rules remain unchanged.
- Focus remains depth-slab based. It does not force the entire focused actor silhouette to stay sharp.
- Focused pixels inside `[SharpNearDistance, SharpFarDistance]` remain exact scene color.
- Pixels outside the sharp slab ease quadratically toward `MaximumBlurStrength` over `BlurTransitionDistance`.
- Blur samples never cross between the near, sharp, and far depth regions.
- Camera UI remains outside the scene post-process and stays sharp.

## Root cause

The current material generator writes both color intermediates at half resolution, averages foreground and background color before depth rejection, and reuses UVs calculated for one render target when sampling another. The horizontal and vertical passes also derive offsets from `View.BufferSizeAndInvSize` while operating on different target resolutions. These choices cause color/depth disagreement, asymmetric radii, edge bleeding, and visible upsampling artifacts.

## Rendering architecture

Keep the three existing material asset paths to preserve serialized references, but change their runtime roles:

1. `M_PP_CameraFocusPrefilter` becomes a full-resolution horizontal depth-aware Gaussian pass and writes `FocusBlurHorizontal`.
2. `M_PP_CameraFocusBlur` becomes a full-resolution vertical depth-aware Gaussian pass and writes `FocusBlurred`.
3. `M_PP_CameraFocusComposite` recalculates the exact full-resolution blur mask and composites `FocusBlurred` over the untouched `PostProcessInput0`.

No unconditional color downsample or box prefilter remains.

For every sample, the shader starts from viewport UV, offsets it in full-resolution source pixels, and independently converts that viewport coordinate for the color and depth texture IDs. Each converted UV is clamped with `ClampSceneTextureUV`. This keeps color and depth on the same displayed pixel even when their render-target extents differ.

## Depth-aware sampling

Each horizontal and vertical sample uses the original full-resolution scene depth at the same viewport coordinate as its color sample.

- Classify the center and sample as near, sharp, or far relative to the current sharp slab.
- Reject samples from a different class with zero weight.
- Within the same class, reduce weight using an exponential bilateral term based on `DepthRejectionDistance`.
- Normalize the surviving Gaussian weights so silhouettes do not darken.

`DepthRejectionDistance` is independent from `BlurTransitionDistance`; changing how quickly blur grows must not also change edge preservation.

## Runtime parameters

The component continues to send:

- `SharpNearDistance`
- `SharpFarDistance`
- `BlurTransitionDistance`
- `MaximumBlurStrength`
- `MaximumBlurRadiusPixels`

It additionally sends:

- `DepthRejectionDistance` (default 100 cm)

The first two material instances now receive the complete parameter set. The composite needs only the focus-mask parameters but may expose the common contract for consistent inspection.

## Compatibility

- Retain the existing material asset paths and soft-object property names.
- Do not modify `BP_OrbitViewCharacter_Legacy`, evidence Blueprints, the prototype map, or investigation data.
- Continue running after tonemapping so the pipeline remains LDR and the existing camera UI behavior is unchanged.
- Keep the built-in depth of field disabled while the custom blur is active.

## Verification

- Material automation tests require both intermediates to use a `1x1` divisor and require the horizontal output name `FocusBlurHorizontal`.
- Asset tests require `DepthRejectionDistance` and clamped User Scene Texture inputs.
- Camera automation tests must remain green.
- Render the prototype at 1280x720 and 1920x1080 with focused curtain and snow-globe targets.
- Confirm the sharp slab aligns with geometry, the blur is isotropic, foreground/background colors do not cross silhouettes, and the camera UI remains sharp.
- Use `r.PostProcessing.UserSceneTextureDebug 1` when diagnosing pass extents.
