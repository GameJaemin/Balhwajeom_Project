import unreal


ASSET_PATHS = [
    "/Game/Balhwajeom/Characters/Player/Dog_Human",
    "/Game/Balhwajeom/Characters/Player/Dog_Idle_Anim",
    "/Game/Balhwajeom/Characters/Player/Dog_Walking_Anim",
    "/Game/Balhwajeom/Blueprints/CameraSystem/BP_OrbitViewCharacter",
]

for path in ASSET_PATHS:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError(f"Missing required asset: {path}")
    unreal.log(f"DOG_ORBIT_INSPECT Asset={asset.get_path_name()} Class={asset.get_class().get_name()}")

    for property_name in ("skeleton", "imported_bounds", "sequence_length", "rate_scale"):
        try:
            value = asset.get_editor_property(property_name)
            unreal.log(f"DOG_ORBIT_INSPECT Property={property_name} Value={value}")
        except Exception:
            pass

    for method_name in ("get_bounds", "get_bounding_box"):
        try:
            value = getattr(asset, method_name)()
            unreal.log(f"DOG_ORBIT_INSPECT Method={method_name} Value={value}")
        except Exception:
            pass

idle = unreal.EditorAssetLibrary.load_asset(ASSET_PATHS[1])
walk = unreal.EditorAssetLibrary.load_asset(ASSET_PATHS[2])
idle_skeleton = idle.get_editor_property("skeleton")
walk_skeleton = walk.get_editor_property("skeleton")
if idle_skeleton != walk_skeleton:
    raise RuntimeError(
        f"Idle/walk skeleton mismatch: {idle_skeleton.get_path_name()} != {walk_skeleton.get_path_name()}"
    )

unreal.log("DOG_ORBIT_INSPECT Result=Success")
