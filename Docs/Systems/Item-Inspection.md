# Item Inspection

The project uses the reusable `ItemInspector`, `JMInteraction`, and `JMGameplayEvent` plugins from `Reuse_Plugin` commit `7b52cb18724375a378f04eef5fa1b2cedb9935f3`.

## Quick manual test

1. Open `/Game/Balhwajeom/Tests/ItemInspection/L_ItemInspectionTest`.
2. Start PIE with the active viewport focused.
3. The large key named `F_Key_Inspection_Test` is in front of the player. Aim the center of the screen at it and press `F`.
4. The preview should travel from the key's projected world rect into the inspector panel while rotating to its authored front view.
5. Drag inside the preview with the left mouse button and use the wheel to rotate and zoom.
6. Press `Escape` or the close button. The preview should return to the key and restore movement, look, cursor, pause, and hidden state.

`F` is not hardcoded by Item Inspector. `ABalhwajeomCameraCharacter` supplies the existing `/Game/Balhwajeom/Input/IMC_Interaction` and `/Game/Balhwajeom/Input/IA_Interact` assets to `UPlayerInteractionComponent`. The editor automation test asserts that this mapping binds `F` to `IA_Interact`.

## Authoring another inspectable

Add `JMInspectableComponent` to any Actor whose visible collision blocks `Visibility`. Assign a `JMItemInspectionData` asset and its `PreviewMesh`. The project interaction trace will select the highest-priority enabled inspection component within 500 cm and execute it through the existing interact action.

The test assets are:

- `/Game/Balhwajeom/Tests/ItemInspection/DA_KeyInspection`
- `/Game/Balhwajeom/Tests/ItemInspection/BP_KeyInspection`
- `/Game/Balhwajeom/Tests/ItemInspection/L_ItemInspectionTest`

## Evidence Actor integration

Every `ABalhwajeomEvidenceActor` now owns an inherited `ItemInspectionComponent`. In an Evidence Blueprint, enable **Inspection > 3D > Enable 3D Inspection**. Assigning **Item Inspection Data** is optional: when it is empty, or when its Preview Mesh/text fields are empty, the actor creates per-instance runtime data from its `EvidenceMesh`, evidence name, Object ID, and current interaction text.

On `F`, the existing investigation interaction runs first so its state transition, keyword award, and display text are preserved. The same input then opens the 3D inspector. If the Evidence interaction is no longer available, the configured 3D inspection remains available as a read-only view.

Project defaults are available under **Project Settings > JM Plugins > Item Inspector**. Per-request settings override Data Asset settings, which override project defaults.
