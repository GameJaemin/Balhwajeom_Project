# Camera Focus and Blur Design

## Goal

Replace the photo camera's zoom-scaled, per-state preferred-distance model with a camera-owned fixed focus range, per-object range offsets, exact center-ray target selection, stable target retention, and a depth-driven Ease-In blur.

## Approved behavior

- Field of view controls zoom presentation only. It never changes focus eligibility, target focal distance, sharp-region distances, or blur transition distances.
- A target is mechanically photographable only while its `CameraFocusPoint` is inside its effective range and the viewport-center visibility ray resolves to that target.
- Evidence-state authorization remains a separate gate: `bCanCapture` must be true, `PhotoID` must be set and registered, and the photo must not already have been captured.
- The former 70% projected-bounds requirement is removed from capture eligibility.
- Visual focus is retained for 0.1 seconds after a target temporarily stops satisfying center/range visibility. The shutter always performs a fresh strict check and is never authorized by the grace period.
- A focused target uses the camera-to-`CameraFocusPoint` distance as its target focal distance.
- With no focused target, the default target focal distance is the midpoint of the camera minimum and maximum focus distances, and the entire minimum-to-maximum interval is the sharp region.
- Focus-region bounds transition smoothly with the same application speed so target changes and target loss do not pop.
- Blur outside the sharp region follows a clamped quadratic Ease-In curve and reaches the configured maximum strength after the configured transition distance.

## Ownership and data model

### `UBalhwajeomPhotoCameraComponent`

The player camera owns the global rules:

- `MinimumFocusDistance` (default 400 cm)
- `MaximumFocusDistance` (default 1000 cm)
- `BlurStartDistance` (default 100 cm)
- `BlurTransitionDistance` (default 500 cm)
- `MaximumBlurStrength` (normalized 0..1, default 0.6)
- `FocusApplicationSpeed` (default 8)
- `FocusTargetGracePeriod` (default 0.1 seconds)
- existing zoom limits and step, with no focus-distance coupling

The 400..1000 cm defaults preserve the old struct defaults of 700 cm preferred distance with a 300 cm tolerance.

### `FEvidenceStateDefinition` and `ABalhwajeomEvidenceActor`

The evidence state owns shared/state-dependent offsets:

- `MinimumFocusDistanceOffset` (default 0 cm)
- `MaximumFocusDistanceOffset` (default 0 cm)

The actor keeps matching fallback properties for legacy/unregistered targets. `CameraFocusPoint` remains a scene component authored on the actor Blueprint or placed instance.

Existing `PreferredFocusDistance`, `FocusDistanceTolerance`, and `bScaleFocusDistanceWithZoom` fields remain deprecated for asset compatibility but do not affect the migrated investigation path.

Effective ranges are:

```text
EffectiveMin = max(0, MinimumFocusDistance + MinimumFocusDistanceOffset)
EffectiveMax = max(0, MaximumFocusDistance + MaximumFocusDistanceOffset)
```

Data with `EffectiveMin > EffectiveMax` is invalid. Invalid targets are rejected and logged once per focus evaluation rather than silently swapping the bounds.

## Target selection

Every focus evaluation performs one complex `ECC_Visibility` trace through the exact viewport center.

1. Resolve the first blocking hit through its actor/attachment owner chain to an object implementing `UBalhwajeomCameraTargetInterface`.
2. Request its target information and `CameraFocusPoint`.
3. Reject it if its focus-point distance is outside its effective range.
4. Accept it as the strict target otherwise.

No off-center silhouette candidate becomes the active focus target. The first blocking object therefore provides the occlusion rule naturally.

The last valid visual target is retained for at most 0.1 seconds when the strict target disappears. If it becomes valid again within that period, focus remains continuous. A different valid strict target replaces it immediately. After the grace interval expires, the visual target clears.

## Focus and blur state

The component maintains current and desired sharp-region boundaries:

- Focused target: `[TargetDistance - BlurStartDistance, TargetDistance + BlurStartDistance]`
- No target: `[MinimumFocusDistance, MaximumFocusDistance]`

The desired focal distance is the target distance or the global range midpoint. Current focal distance and sharp-region boundaries move toward their desired values using `FInterpTo` with `FocusApplicationSpeed`.

The post-process blur material evaluates each pixel using camera-space scene depth:

```text
OutsideDistance = max(CurrentSharpNear - PixelDepth, PixelDepth - CurrentSharpFar, 0)
Progress = saturate(OutsideDistance / max(BlurTransitionDistance, epsilon))
BlurStrength = MaximumBlurStrength * Progress^2
```

The material applies a fixed nine-tap symmetric screen-space blur whose sample radius is multiplied by `BlurStrength`. The normalized maximum strength remains designer-facing; kernel size stays an implementation detail.

The old physical-camera F-stop override is disabled while this material is active so two independent blur models do not stack. Camera-mode exit restores the original post-process settings and blendables.

## Capture checks

The shutter refreshes the strict target without grace and validates, in order:

1. viewport-center visibility ray resolves to a camera target;
2. target `CameraFocusPoint` is inside its valid effective range;
3. target snapshot resolves to the current investigation object/state;
4. state permits capture and supplies a photo ID;
5. no save is pending and the photo has not already been captured.

The projected coverage check is not part of capture eligibility.

## UI behavior

The existing `WBP_EvidenceFocusGuide` remains the visual surface. While a strict or grace-retained visual target exists, its confirmed guide location is the projected `CameraFocusPoint`. The existing camera/check status and `NearLabel` data flow remain unchanged. Grace retention may keep the visual focus briefly, but it never permits a stale capture.

## Testing

- Pure automation tests cover range calculation, invalid offset ranges, FOV independence, sharp-region calculation, Ease-In progress, and 0.1-second target retention state.
- Camera integration tests cover center-ray-only acquisition, occlusion, removal of the 70% gate, strict shutter validation during grace, and `CameraFocusPoint`-based focal distance.
- Asset tests verify that the post-process material exists, uses the Post Process domain, and exposes the runtime parameter names.
- Existing investigation, interaction, UI asset, and prototype capture tests remain green.

