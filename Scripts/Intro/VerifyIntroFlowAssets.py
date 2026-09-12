import unreal


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    menu = unreal.load_asset("/Game/Balhwajeom/UI/Title/WBP_MainMenu")
    fade = unreal.load_asset("/Game/Balhwajeom/UI/Title/WBP_ScreenFade")
    flow_bp = unreal.load_asset(
        "/Game/Balhwajeom/Blueprints/Intro/BP_IntroFlowController"
    )
    require(menu is not None, "WBP_MainMenu is missing")
    require(fade is not None, "WBP_ScreenFade is missing")
    require(flow_bp is not None, "BP_IntroFlowController is missing")

    require(
        unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path(
            "/Game/Balhwajeom/UI/Title/WBP_MainMenu.WBP_MainMenu"
        ),
        "WBP_MainMenu inspection failed",
    )
    require(
        unreal.TabletWidgetBlueprintLibrary.inspect_widget_blueprint_by_path(
            "/Game/Balhwajeom/UI/Title/WBP_ScreenFade.WBP_ScreenFade"
        ),
        "WBP_ScreenFade inspection failed",
    )

    world = unreal.EditorLoadingAndSavingUtils.load_map(
        "/Game/Balhwajeom/Maps/Prototype/L_InvestigationPrototype_Jung"
    )
    require(world is not None, "Target map failed to load")
    flow_class = unreal.load_class(
        None,
        "/Game/Balhwajeom/Blueprints/Intro/BP_IntroFlowController.BP_IntroFlowController_C",
    )
    require(flow_class is not None, "Flow class failed to load")
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor.get_class() == flow_class
    ]
    require(len(actors) == 1, "Expected exactly one intro flow actor, got %d" % len(actors))
    unreal.log("INTRO_FLOW_VERIFY Result=Success Actor=%s" % actors[0].get_name())


if __name__ == "__main__":
    main()
