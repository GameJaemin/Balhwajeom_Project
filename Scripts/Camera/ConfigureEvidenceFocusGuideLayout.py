import unreal


if not unreal.TabletWidgetBlueprintLibrary.configure_evidence_focus_guide_layout():
    raise RuntimeError("Failed to configure WBP_EvidenceFocusGuide layout")

unreal.log("EVIDENCE_FOCUS_GUIDE_LAYOUT_CONFIGURE Result=Success")
