import unreal


INPUT_PATH = "/Game/Balhwajeom/Core/Input"
ACTION_PATH = f"{INPUT_PATH}/IA_Tablet"
CONTEXT_PATH = f"{INPUT_PATH}/IMC_Tablet"


def load_or_create(asset_path, asset_name, asset_class, factory_class):
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing:
        return existing

    factory = factory_class()
    if hasattr(factory, "set_editor_property"):
        property_name = (
            "input_action_class"
            if asset_class == unreal.InputAction
            else "input_mapping_context_class"
        )
        try:
            factory.set_editor_property(property_name, asset_class)
        except Exception:
            pass

    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        INPUT_PATH,
        asset_class,
        factory,
    )


unreal.EditorAssetLibrary.make_directory(INPUT_PATH)

tablet_action = load_or_create(
    ACTION_PATH,
    "IA_Tablet",
    unreal.InputAction,
    unreal.InputAction_Factory,
)
tablet_action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
tablet_action.set_editor_property("consume_input", True)
tablet_action.set_editor_property("trigger_when_paused", False)

tablet_context = load_or_create(
    CONTEXT_PATH,
    "IMC_Tablet",
    unreal.InputMappingContext,
    unreal.InputMappingContext_Factory,
)
tablet_context.unmap_all()
tablet_key = unreal.Key()
tablet_key.set_editor_property("key_name", "T")
tablet_context.map_key(tablet_action, tablet_key)

unreal.EditorAssetLibrary.save_asset(ACTION_PATH, only_if_is_dirty=False)
unreal.EditorAssetLibrary.save_asset(CONTEXT_PATH, only_if_is_dirty=False)

unreal.log("Tablet prototype input assets are ready: IA_Tablet, IMC_Tablet (T key).")
