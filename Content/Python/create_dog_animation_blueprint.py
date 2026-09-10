import unreal


if not unreal.DogAnimationBlueprintLibrary.create_dog_animation_blueprint():
    raise RuntimeError("ABP_Dog creation or validation failed; inspect the Unreal log")

unreal.log("ABP_DOG_PYTHON Result=Success")
