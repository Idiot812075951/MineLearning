"""Recreate the Gunner dissolve adapter with a fresh material shader state."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
ASSET_NAME = 'M_RadiantDissolve_Gunner'
ASSET_PATH = ROOT + '/' + ASSET_NAME
editing = u.MaterialEditingLibrary
library = u.EditorAssetLibrary


def custom_input(name):
    value = u.CustomInput()
    value.set_editor_property('input_name', name)
    return value


def node(material, cls, x, y, **properties):
    result = editing.create_material_expression(material, cls, x, y)
    for key, value in properties.items():
        result.set_editor_property(key, value)
    return result


def scalar(material, name, default, x, y, group='Radiant Dissolve'):
    return node(
        material,
        u.MaterialExpressionScalarParameter,
        x,
        y,
        parameter_name=name,
        default_value=default,
        group=group,
    )


def texture(material, name, asset_path, sampler, x, y):
    return node(
        material,
        u.MaterialExpressionTextureSampleParameter2D,
        x,
        y,
        parameter_name=name,
        texture=library.load_asset(asset_path),
        sampler_type=sampler,
        group='Gunner Surface',
    )


def connect(source, destination, input_name, output_name=''):
    if not editing.connect_material_expressions(source, output_name, destination, input_name):
        raise RuntimeError(
            f'Could not connect {source.get_name()} -> {destination.get_name()}.{input_name}'
        )


bp = library.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)
old_adapter = library.load_asset(ASSET_PATH)
if old_adapter:
    u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(old_adapter)
    if not library.delete_asset(ASSET_PATH):
        raise RuntimeError('Could not remove the stale Gunner adapter')

factory = u.MaterialFactoryNew()
material = u.AssetToolsHelpers.get_asset_tools().create_asset(
    ASSET_NAME,
    ROOT,
    u.Material,
    factory,
)
if not material:
    raise RuntimeError('Could not create the fresh Gunner adapter')

material.set_editor_property('blend_mode', u.BlendMode.BLEND_MASKED)
material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_DEFAULT_LIT)
material.set_editor_property('used_with_skeletal_mesh', True)
material.set_editor_property('automatically_set_usage_in_editor', True)
material.set_editor_property('opacity_mask_clip_value', 0.32)

base = texture(
    material,
    'GunnerBaseColor',
    '/Game/MineLearning/Characters/Gunner/Textures/T_Gunner_BaseColor',
    u.MaterialSamplerType.SAMPLERTYPE_COLOR,
    -900,
    -260,
)
roughness = texture(
    material,
    'GunnerRoughness',
    '/Game/MineLearning/Characters/Gunner/Textures/T_Gunner_Roughness',
    # The texture asset is authored as Color.  A sampler mismatch makes SM6
    # compile fail and Unreal displays the default white fallback material.
    u.MaterialSamplerType.SAMPLERTYPE_COLOR,
    -900,
    540,
)
normal = texture(
    material,
    'GunnerNormal',
    '/Game/MineLearning/Characters/Gunner/Textures/T_Gunner_Normal',
    u.MaterialSamplerType.SAMPLERTYPE_NORMAL,
    -900,
    700,
)

function_call = node(
    material,
    u.MaterialExpressionMaterialFunctionCall,
    -650,
    100,
    material_function=library.load_asset(ROOT + '/MF_RadiantDissolve'),
)
outline = scalar(material, 'OutlineAmount', 0.0, -650, 410)
opacity_fade = scalar(material, 'OpacityFade', 1.0, -650, 520)
intensity = scalar(material, 'HeatIntensity', 70.0, -650, 630)
fresnel = node(material, u.MaterialExpressionFresnel, -400, 430, exponent=1.35)

surface = node(
    material,
    u.MaterialExpressionCustom,
    -100,
    -180,
    description='Preserve Gunner surface while heating',
    output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
)
surface.set_editor_property(
    'inputs',
    [custom_input(name) for name in ['Base', 'Masks', 'Outline', 'Fresnel']],
)
surface.set_editor_property(
    'code',
    'float3 hot=lerp(Base,Base*.24+float3(.31,.002,.008),Masks.x*.9); '
    'float shell=saturate(Outline)*smoothstep(.34,.82,saturate(Fresnel)); '
    'return lerp(hot,float3(.44,.001,.01),shell*.72);',
)
connect(base, surface, 'Base', 'RGB')
connect(function_call, surface, 'Masks')
connect(outline, surface, 'Outline')
connect(fresnel, surface, 'Fresnel')

emission = node(
    material,
    u.MaterialExpressionCustom,
    -100,
    80,
    description='Gunner heat edge and early red silhouette',
    output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
)
emission.set_editor_property(
    'inputs',
    [custom_input(name) for name in ['Masks', 'Intensity', 'Outline', 'Fresnel']],
)
emission.set_editor_property(
    'code',
    'float front=pow(saturate(Masks.y),1.45); '
    'float shell=saturate(Outline)*smoothstep(.30,.78,saturate(Fresnel)); '
    'return Intensity*(pow(saturate(Masks.x),1.45)*float3(1,.008,.018)*.30 '
    '+front*float3(1,.42,.58)*2.0+shell*.90*float3(1,.018,.055));',
)
connect(function_call, emission, 'Masks')
connect(intensity, emission, 'Intensity')
connect(outline, emission, 'Outline')
connect(fresnel, emission, 'Fresnel')

remaining = node(
    material,
    u.MaterialExpressionComponentMask,
    -390,
    880,
    r=False,
    g=False,
    b=True,
    a=False,
)
connect(function_call, remaining, '')
world_position = node(material, u.MaterialExpressionWorldPosition, -390, 1010)
fade_dither = node(
    material,
    u.MaterialExpressionCustom,
    -100,
    1010,
    description='Blue-noise Gunner silhouette fade',
    output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
)
fade_dither.set_editor_property(
    'inputs',
    [custom_input('Fade'), custom_input('Position')],
)
fade_dither.set_editor_property(
    'code',
    'float n=frac(sin(dot(Position.xyz,float3(12.9898,78.233,37.719)))*43758.5453); '
    'return (Fade<=.001)?0:step(n,saturate(Fade));',
)
connect(opacity_fade, fade_dither, 'Fade')
connect(world_position, fade_dither, 'Position')

held_mask = node(
    material,
    u.MaterialExpressionCustom,
    -100,
    830,
    description='Keep Gunner outline after interior erosion',
    output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
)
held_mask.set_editor_property(
    'inputs',
    [custom_input(name) for name in ['Remaining', 'Outline', 'Fresnel']],
)
held_mask.set_editor_property(
    'code',
    'float shell=step(.42,saturate(Fresnel))*saturate(Outline); '
    'return max(Remaining,shell);',
)
connect(remaining, held_mask, 'Remaining')
connect(outline, held_mask, 'Outline')
connect(fresnel, held_mask, 'Fresnel')
faded = node(material, u.MaterialExpressionMultiply, 170, 890)
connect(held_mask, faded, 'A')
connect(fade_dither, faded, 'B')

editing.connect_material_property(surface, '', u.MaterialProperty.MP_BASE_COLOR)
editing.connect_material_property(emission, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
editing.connect_material_property(faded, '', u.MaterialProperty.MP_OPACITY_MASK)
editing.connect_material_property(roughness, 'R', u.MaterialProperty.MP_ROUGHNESS)
editing.connect_material_property(normal, 'RGB', u.MaterialProperty.MP_NORMAL)

editing.recompile_material(material)
library.save_loaded_asset(material)

BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
cdo.set_editor_property('RadiantSkeletalFallbackMaterial', material)
BP.compile_blueprint(bp, warnings_as_errors=True)
library.save_loaded_asset(bp)

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() == 'BP_RadiantDissolve_Test_C':
        actor.set_editor_property('RadiantSkeletalFallbackMaterial', material)

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_GUNNER_ADAPTER_REBUILT_FRESH')
