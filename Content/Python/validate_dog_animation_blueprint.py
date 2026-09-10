import unreal


if not unreal.DogAnimationBlueprintLibrary.validate_dog_animation_blueprint():
    raise RuntimeError("ABP_Dog validation failed; inspect the Unreal log")

unreal.log("ABP_DOG_VALIDATION_PYTHON Result=Success")
