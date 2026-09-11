import unreal


MATERIAL_ROOT = "/Game/Balhwajeom/Camera/Materials"
FAR_HORIZONTAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusPrefilter"
FAR_VERTICAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusBlur"
NEAR_HORIZONTAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusNearHorizontal"
NEAR_VERTICAL_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusNearVertical"
COMPOSITE_PATH = f"{MATERIAL_ROOT}/M_PP_CameraFocusComposite"

FOCUS_DEFAULTS = {
    "SharpNearDistance": 400.0,
    "SharpFarDistance": 1000.0,
    "BlurTransitionDistance": 500.0,
    "MaximumBlurStrength": 0.6,
    "MaximumBlurRadiusPixels": 12.0,
    "NearBlurRadiusScale": 1.25,
    "FarBlurRadiusScale": 1.0,
}


def create_or_reset_material(asset_path, blendable_priority):
    material = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )
    if material is None:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(
            asset_path.rsplit("/", 1)[-1],
            asset_path.rsplit("/", 1)[0],
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if material is None:
        raise RuntimeError(f"Could not create {asset_path}")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property(
        "blendable_location",
        unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING,
    )
    material.set_editor_property("blendable_priority", blendable_priority)
    material.set_editor_property("blendable_output_alpha", False)
    material.set_editor_property("user_scene_texture", "")
    material.set_editor_property("user_texture_divisor", unreal.IntPoint(1, 1))
    material.set_editor_property("disable_pre_exposure_scale", True)
    return material


def create_scalar(material, name, default_value, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, x, y
    )
    expression.set_editor_property("parameter_name", name)
    expression.set_editor_property("default_value", default_value)
    return expression


def create_scene_texture(material, texture_id, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionSceneTexture, x, y
    )
    expression.set_editor_property("scene_texture_id", texture_id)
    return expression


def create_user_scene_texture(material, texture_name, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionUserSceneTexture, x, y
    )
    expression.set_editor_property("user_scene_texture", texture_name)
    expression.set_editor_property("filtered", True)
    expression.set_editor_property("clamped", True)
    return expression


def create_custom_input(name):
    custom_input = unreal.CustomInput()
    custom_input.set_editor_property("input_name", name)
    return custom_input


def create_custom(
    material,
    description,
    code,
    input_names,
    output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
    x=-350,
    y=0,
):
    custom = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionCustom, x, y
    )
    custom.set_editor_property("description", description)
    custom.set_editor_property("output_type", output_type)
    custom.set_editor_property(
        "inputs", [create_custom_input(name) for name in input_names]
    )
    custom.set_editor_property("code", code.strip())
    return custom


def connect_inputs(custom, sources):
    for input_name, source in sources.items():
        if not unreal.MaterialEditingLibrary.connect_material_expressions(
            source, "", custom, input_name
        ):
            raise RuntimeError(f"Could not connect {input_name}")


def save_material(material, asset_path):
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        material, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Could not save {asset_path}")
    unreal.log(f"Created {asset_path}")


def finish_color_material(material, output_expression, asset_path):
    if not unreal.MaterialEditingLibrary.connect_material_property(
        output_expression, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError(f"Could not connect color output for {asset_path}")
    save_material(material, asset_path)


def finish_rgba_material(material, output_expression, asset_path):
    material.set_editor_property("blendable_output_alpha", True)
    alpha = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionComponentMask, 220, 120
    )
    alpha.set_editor_property("r", False)
    alpha.set_editor_property("g", False)
    alpha.set_editor_property("b", False)
    alpha.set_editor_property("a", True)
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        output_expression, "", alpha, ""
    ):
        raise RuntimeError(f"Could not connect alpha mask for {asset_path}")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        output_expression, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError(f"Could not connect RGB output for {asset_path}")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        alpha, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError(f"Could not connect alpha output for {asset_path}")
    save_material(material, asset_path)


def create_focus_parameters(material, x=-1000, y=-450):
    parameters = {}
    for index, (name, value) in enumerate(FOCUS_DEFAULTS.items()):
        parameters[name] = create_scalar(material, name, value, x, y + index * 110)
    return parameters


def configure_intermediate(material, texture_name):
    material.set_editor_property("user_scene_texture", texture_name)
    material.set_editor_property("user_texture_divisor", unreal.IntPoint(1, 1))


def build_far_horizontal():
    material = create_or_reset_material(FAR_HORIZONTAL_PATH, 0)
    configure_intermediate(material, "FocusFarHorizontal")
    focus_parameters = create_focus_parameters(material)
    scene_color = create_scene_texture(
        material, unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, -1000, 420
    )
    depth = create_scene_texture(
        material, unreal.SceneTextureId.PPI_SCENE_DEPTH, -1000, 550
    )
    custom = create_custom(
        material,
        "Full-resolution horizontal far-layer Gaussian blur",
        """
float CenterDepth = DepthInput.r;
if (CenterDepth <= SharpFarDistance)
{
    return SceneColorInput.rgb;
}
float FarProgress = saturate((CenterDepth - SharpFarDistance) / max(BlurTransitionDistance, 0.001));
float FarAmount = saturate(MaximumBlurStrength) * FarProgress * FarProgress;
float ResolutionScale = max(GetSceneTextureViewSize(SceneColorInput.ID).y / 1080.0, 0.25);
float RadiusPixels = max(MaximumBlurRadiusPixels * FarBlurRadiusScale, 0.0) * FarAmount * ResolutionScale;
float2 ViewportUV = GetViewportUV(Parameters);
float3 Sum = 0.0;
float WeightSum = 0.0;
[unroll]
for (int Index = -8; Index <= 8; ++Index)
{
    float NormalizedOffset = float(Index) / 8.0;
    float Weight = exp2(-4.0 * NormalizedOffset * NormalizedOffset);
    float2 SampleViewportUV = saturate(ViewportUV + float2(
        GetSceneTextureViewSize(SceneColorInput.ID).z * RadiusPixels * NormalizedOffset, 0.0));
    float2 ColorUV = ClampSceneTextureUV(
        ViewportUVToSceneTextureUV(SampleViewportUV, SceneColorInput.ID), SceneColorInput.ID);
    float2 DepthUV = ClampSceneTextureUV(
        ViewportUVToSceneTextureUV(SampleViewportUV, DepthTexture.ID), DepthTexture.ID);
    float SampleDepth = SceneTextureLookup(DepthUV, DepthTexture.ID, false).r;
    if (SampleDepth > SharpFarDistance)
    {
        Sum += SceneTextureLookup(ColorUV, SceneColorInput.ID, true).rgb * Weight;
        WeightSum += Weight;
    }
}
return WeightSum > 0.0001 ? Sum / WeightSum : SceneColorInput.rgb;
""",
        list(focus_parameters.keys()) + ["SceneColorInput", "DepthInput", "DepthTexture"],
    )
    connect_inputs(
        custom,
        {
            **focus_parameters,
            "SceneColorInput": scene_color,
            "DepthInput": depth,
            "DepthTexture": depth,
        },
    )
    finish_color_material(material, custom, FAR_HORIZONTAL_PATH)


def build_far_vertical():
    material = create_or_reset_material(FAR_VERTICAL_PATH, 1)
    configure_intermediate(material, "FocusFarBlurred")
    focus_parameters = create_focus_parameters(material)
    source = create_user_scene_texture(material, "FocusFarHorizontal", -1000, 420)
    depth = create_scene_texture(
        material, unreal.SceneTextureId.PPI_SCENE_DEPTH, -1000, 550
    )
    custom = create_custom(
        material,
        "Full-resolution vertical far-layer Gaussian blur",
        """
float CenterDepth = DepthInput.r;
if (CenterDepth <= SharpFarDistance)
{
    return SourceColor.rgb;
}
float FarProgress = saturate((CenterDepth - SharpFarDistance) / max(BlurTransitionDistance, 0.001));
float FarAmount = saturate(MaximumBlurStrength) * FarProgress * FarProgress;
float ResolutionScale = max(GetSceneTextureViewSize(SourceColor.ID).y / 1080.0, 0.25);
float RadiusPixels = max(MaximumBlurRadiusPixels * FarBlurRadiusScale, 0.0) * FarAmount * ResolutionScale;
float2 ViewportUV = GetViewportUV(Parameters);
float3 Sum = 0.0;
float WeightSum = 0.0;
[unroll]
for (int Index = -8; Index <= 8; ++Index)
{
    float NormalizedOffset = float(Index) / 8.0;
    float Weight = exp2(-4.0 * NormalizedOffset * NormalizedOffset);
    float2 SampleViewportUV = saturate(ViewportUV + float2(
        0.0, GetSceneTextureViewSize(SourceColor.ID).w * RadiusPixels * NormalizedOffset));
    float2 ColorUV = ClampSceneTextureUV(
        ViewportUVToSceneTextureUV(SampleViewportUV, SourceColor.ID), SourceColor.ID);
    float2 DepthUV = ClampSceneTextureUV(
        ViewportUVToSceneTextureUV(SampleViewportUV, DepthTexture.ID), DepthTexture.ID);
    float SampleDepth = SceneTextureLookup(DepthUV, DepthTexture.ID, false).r;
    if (SampleDepth > SharpFarDistance)
    {
        Sum += SceneTextureLookup(ColorUV, SourceColor.ID, true).rgb * Weight;
        WeightSum += Weight;
    }
}
return WeightSum > 0.0001 ? Sum / WeightSum : SourceColor.rgb;
""",
        list(focus_parameters.keys()) + ["SourceColor", "DepthInput", "DepthTexture"],
    )
    connect_inputs(
        custom,
        {
            **focus_parameters,
            "SourceColor": source,
            "DepthInput": depth,
            "DepthTexture": depth,
        },
    )
    finish_color_material(material, custom, FAR_VERTICAL_PATH)


def build_near_horizontal():
    material = create_or_reset_material(NEAR_HORIZONTAL_PATH, 2)
    configure_intermediate(material, "FocusNearHorizontal")
    focus_parameters = create_focus_parameters(material)
    scene_color = create_scene_texture(
        material, unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, -1000, 420
    )
    depth = create_scene_texture(
        material, unreal.SceneTextureId.PPI_SCENE_DEPTH, -1000, 550
    )
    custom = create_custom(
        material,
        "Full-resolution horizontal premultiplied near contribution and coverage",
        """
float ResolutionScale = max(GetSceneTextureViewSize(SceneColorInput.ID).y / 1080.0, 0.25);
float MaxRadiusPixels = max(MaximumBlurRadiusPixels * NearBlurRadiusScale, 0.0)
    * saturate(MaximumBlurStrength) * ResolutionScale;
float2 ViewportUV = GetViewportUV(Parameters);
float3 PremultipliedColor = 0.0;
float CoverageSum = 0.0;
float KernelWeightSum = 0.0;
[unroll]
for (int Index = -8; Index <= 8; ++Index)
{
    float NormalizedOffset = float(Index) / 8.0;
    float Weight = exp2(-4.0 * NormalizedOffset * NormalizedOffset);
    float2 SampleViewportUV = saturate(ViewportUV + float2(
        GetSceneTextureViewSize(SceneColorInput.ID).z * MaxRadiusPixels * NormalizedOffset, 0.0));
    float2 ColorUV = ClampSceneTextureUV(
        ViewportUVToSceneTextureUV(SampleViewportUV, SceneColorInput.ID), SceneColorInput.ID);
    float2 DepthUV = ClampSceneTextureUV(
        ViewportUVToSceneTextureUV(SampleViewportUV, DepthTexture.ID), DepthTexture.ID);
    float SampleDepth = SceneTextureLookup(DepthUV, DepthTexture.ID, false).r;
    float NearProgress = saturate(
        (SharpNearDistance - SampleDepth) / max(BlurTransitionDistance, 0.001));
    float NearCoverage = saturate(MaximumBlurStrength) * NearProgress * NearProgress;
    float SourceRadius = max(MaximumBlurRadiusPixels * NearBlurRadiusScale, 0.0)
        * NearCoverage * ResolutionScale;
    float SampleOffsetPixels = abs(NormalizedOffset) * MaxRadiusPixels;
    float Support = saturate(SourceRadius - SampleOffsetPixels + 1.0);
    float Contribution = Weight * NearCoverage * Support;
    PremultipliedColor += SceneTextureLookup(ColorUV, SceneColorInput.ID, true).rgb * Contribution;
    CoverageSum += Contribution;
    KernelWeightSum += Weight;
}
return float4(
    PremultipliedColor / max(KernelWeightSum, 0.0001),
    saturate(CoverageSum / max(KernelWeightSum, 0.0001)));
""",
        list(focus_parameters.keys()) + ["SceneColorInput", "DepthTexture"],
        output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT4,
    )
    connect_inputs(
        custom,
        {
            **focus_parameters,
            "SceneColorInput": scene_color,
            "DepthTexture": depth,
        },
    )
    finish_rgba_material(material, custom, NEAR_HORIZONTAL_PATH)


def build_near_vertical():
    material = create_or_reset_material(NEAR_VERTICAL_PATH, 3)
    configure_intermediate(material, "FocusNearBlurred")
    focus_parameters = create_focus_parameters(material)
    source = create_user_scene_texture(material, "FocusNearHorizontal", -1000, 420)
    custom = create_custom(
        material,
        "Full-resolution vertical premultiplied near contribution and coverage",
        """
float ResolutionScale = max(GetSceneTextureViewSize(NearInput.ID).y / 1080.0, 0.25);
float MaxRadiusPixels = max(MaximumBlurRadiusPixels * NearBlurRadiusScale, 0.0)
    * saturate(MaximumBlurStrength) * ResolutionScale;
float2 ViewportUV = GetViewportUV(Parameters);
float3 PremultipliedColor = 0.0;
float NearCoverage = 0.0;
float KernelWeightSum = 0.0;
[unroll]
for (int Index = -8; Index <= 8; ++Index)
{
    float NormalizedOffset = float(Index) / 8.0;
    float Weight = exp2(-4.0 * NormalizedOffset * NormalizedOffset);
    float2 SampleViewportUV = saturate(ViewportUV + float2(
        0.0, GetSceneTextureViewSize(NearInput.ID).w * MaxRadiusPixels * NormalizedOffset));
    float2 NearUV = ClampSceneTextureUV(
        ViewportUVToSceneTextureUV(SampleViewportUV, NearInput.ID), NearInput.ID);
    float4 NearSample = SceneTextureLookup(NearUV, NearInput.ID, true);
    PremultipliedColor += NearSample.rgb * Weight;
    NearCoverage += NearSample.a * Weight;
    KernelWeightSum += Weight;
}
return float4(
    PremultipliedColor / max(KernelWeightSum, 0.0001),
    saturate(NearCoverage / max(KernelWeightSum, 0.0001)));
""",
        list(focus_parameters.keys()) + ["NearInput"],
        output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT4,
    )
    connect_inputs(custom, {**focus_parameters, "NearInput": source})
    finish_rgba_material(material, custom, NEAR_VERTICAL_PATH)


def build_composite():
    material = create_or_reset_material(COMPOSITE_PATH, 4)
    focus_parameters = create_focus_parameters(material)
    scene_color = create_scene_texture(
        material, unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, -1000, 420
    )
    far_blurred = create_user_scene_texture(
        material, "FocusFarBlurred", -1000, 550
    )
    near_blurred = create_user_scene_texture(
        material, "FocusNearBlurred", -1000, 680
    )
    depth = create_scene_texture(
        material, unreal.SceneTextureId.PPI_SCENE_DEPTH, -1000, 810
    )
    custom = create_custom(
        material,
        "Full-resolution ordered far then near focus composite",
        """
float CenterDepth = DepthInput.r;
float FarProgress = saturate(
    (CenterDepth - SharpFarDistance) / max(BlurTransitionDistance, 0.001));
float FarAmount = saturate(MaximumBlurStrength) * FarProgress * FarProgress;
float3 BaseColor = lerp(SceneColorInput.rgb, FarInput.rgb, FarAmount);
float NearCoverage = saturate(NearInput.a);
float3 NearColor = NearInput.rgb / max(NearCoverage, 0.0001);
return lerp(BaseColor, NearColor, NearCoverage);
""",
        list(focus_parameters.keys())
        + ["SceneColorInput", "FarInput", "NearInput", "DepthInput"],
    )
    connect_inputs(
        custom,
        {
            **focus_parameters,
            "SceneColorInput": scene_color,
            "FarInput": far_blurred,
            "NearInput": near_blurred,
            "DepthInput": depth,
        },
    )
    finish_color_material(material, custom, COMPOSITE_PATH)


build_far_horizontal()
build_far_vertical()
build_near_horizontal()
build_near_vertical()
build_composite()
unreal.log("Created layered full-resolution camera depth-of-field pipeline")
