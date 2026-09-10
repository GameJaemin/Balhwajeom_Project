import unreal


widget_asset_path = "/Game/Balhwajeom/UI/PhotoStory/WBP_PhotoWorldStory"
widget_blueprint = unreal.load_asset(widget_asset_path)
if widget_blueprint is None:
    raise RuntimeError("Missing photo story widget: %s" % widget_asset_path)

widget_class = widget_blueprint.generated_class()
if widget_class is None:
    raise RuntimeError("Photo story widget has no generated class")

actor_defaults = unreal.get_default_object(unreal.PhotoWorldStoryActor)
configured_class = actor_defaults.get_editor_property("story_widget_class")
if configured_class != widget_class:
    raise RuntimeError(
        "PhotoWorldStoryActor uses %s instead of %s"
        % (configured_class.get_path_name(), widget_class.get_path_name())
    )

widget_defaults = unreal.get_default_object(widget_class)
font = widget_defaults.get_editor_property("story_font")
third_person_font_multiplier = widget_defaults.get_editor_property(
    "third_person_scale_multiplier"
)
scale_transition_duration = widget_defaults.get_editor_property(
    "third_person_scale_transition_duration"
)
if third_person_font_multiplier < 1.0:
    raise RuntimeError("Third-person photo story font multiplier must be at least 1.0")
if scale_transition_duration < 0.0:
    raise RuntimeError("Photo story scale transition duration cannot be negative")

unreal.log(
    "VALIDATE_PHOTO_STORY_WIDGET Result=Success Class=%s FontSize=%s Color=%s "
    "ThirdPersonFontMultiplier=%s TransitionDuration=%s"
    % (
        widget_class.get_path_name(),
        font.size,
        widget_defaults.get_editor_property("story_color"),
        third_person_font_multiplier,
        scale_transition_duration,
    )
)
