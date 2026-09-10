import unreal


asset_folder = "/Game/Balhwajeom/UI/PhotoStory"
asset_name = "WBP_PhotoWorldStory"
asset_path = asset_folder + "/" + asset_name

blueprint = unreal.load_asset(asset_path)
if blueprint is None:
    unreal.EditorAssetLibrary.make_directory(asset_folder)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.PhotoWorldStoryWidget)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        asset_folder,
        unreal.WidgetBlueprint,
        factory,
    )

if blueprint is None:
    raise RuntimeError("Could not create or load %s" % asset_path)

if not unreal.TabletWidgetBlueprintLibrary.redesign_photo_world_story_widget():
    raise RuntimeError("Could not add editable StoryText to %s" % asset_path)

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
    raise RuntimeError("Could not save %s" % asset_path)

generated_class = blueprint.generated_class()
defaults = unreal.get_default_object(generated_class)
unreal.log(
    "PHOTO_STORY_WIDGET Result=Success Asset=%s Class=%s FontSize=%s Color=%s"
    % (
        asset_path,
        generated_class.get_path_name(),
        defaults.get_editor_property("story_font").size,
        defaults.get_editor_property("story_color"),
    )
)
