"""Build source-preserving masked and translucent Gunner dissolve adapters."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
SOURCE = '/Game/MineLearning/Characters/Gunner/Materials/M_Gunner'
MASKED = ROOT + '/M_RadiantDissolve_GunnerSurface'
FADE = ROOT + '/M_RadiantDissolve_GunnerFade'
AK_METAL_SOURCE = '/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Metal'
AK_POLYMER_SOURCE = '/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Polymer'
AK_METAL = ROOT + '/M_RadiantDissolve_AKMetalSurface'
AK_POLYMER = ROOT + '/M_RadiantDissolve_AKPolymerSurface'
editing = u.MaterialEditingLibrary
library = u.EditorAssetLibrary


def custom_input(name):
    value = u.CustomInput()
    value.set_editor_property('input_name', name)
    return value


def node(material, expression_class, x, y, **properties):
    value = editing.create_material_expression(material, expression_class, x, y)
    for key, prop in properties.items():
        value.set_editor_property(key, prop)
    return value


def scalar(material, name, default, x, y):
    return node(material, u.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=default, group='Radiant Dissolve')


def connect(source, destination, input_name, output_name=''):
    if not editing.connect_material_expressions(source, output_name, destination, input_name):
        raise RuntimeError(
            f'Could not connect {source.get_name()} -> {destination.get_name()}.{input_name}'
        )


def build_adapter(source, destination, translucent, used_with_skeletal):
    previous = library.load_asset(destination)
    if previous:
        u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(previous)
        if not library.delete_asset(destination):
            raise RuntimeError(f'Could not remove {destination}')
    material = library.duplicate_asset(source, destination)
    if not material:
        raise RuntimeError(f'Could not duplicate {source} to {destination}')

    base_node = editing.get_material_property_input_node(material, u.MaterialProperty.MP_BASE_COLOR)
    base_output = editing.get_material_property_input_node_output_name(
        material, u.MaterialProperty.MP_BASE_COLOR
    )
    original_emissive = editing.get_material_property_input_node(
        material, u.MaterialProperty.MP_EMISSIVE_COLOR
    )
    original_emissive_output = editing.get_material_property_input_node_output_name(
        material, u.MaterialProperty.MP_EMISSIVE_COLOR
    )
    if not base_node:
        raise RuntimeError('M_Gunner has no Base Color graph to preserve')

    material.set_editor_property(
        'blend_mode',
        u.BlendMode.BLEND_TRANSLUCENT if translucent else u.BlendMode.BLEND_MASKED,
    )
    material.set_editor_property('used_with_skeletal_mesh', used_with_skeletal)
    material.set_editor_property('automatically_set_usage_in_editor', True)
    if translucent:
        material.set_editor_property(
            'translucency_lighting_mode',
            u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING,
        )
    else:
        material.set_editor_property('opacity_mask_clip_value', 0.2)

    masks = node(material, u.MaterialExpressionMaterialFunctionCall, -520, 520,
                 material_function=library.load_asset(ROOT + '/MF_RadiantDissolve'))
    outline = scalar(material, 'OutlineAmount', 0.0, -520, 720)
    opacity_fade = scalar(material, 'OpacityFade', 1.0, -520, 850)
    intensity = scalar(material, 'HeatIntensity', 70.0, -520, 980)
    fresnel = node(material, u.MaterialExpressionFresnel, -280, 760, exponent=1.35)

    surface = node(material, u.MaterialExpressionCustom, 80, 300,
                   description='Preserve Gunner surface while heating',
                   output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
    surface.set_editor_property(
        'inputs', [custom_input(name) for name in ['Base', 'Masks', 'Outline', 'Fresnel']]
    )
    clip_code = (
        '' if translucent else
        'float keep=max(Masks.b,step(.78,saturate(Fresnel))*saturate(Outline)); '
        'clip(keep-.20); '
    )
    surface.set_editor_property(
        'code',
        clip_code + 'float heat=saturate(Masks.x); '
        'float3 hot=lerp(Base,Base*float3(.92,.48,.44)+float3(.13,.002,.006),heat*.38); '
        'float shell=saturate(Outline)*smoothstep(.70,.96,saturate(Fresnel)); '
        'return lerp(hot,float3(.48,.002,.016),shell*.70);',
    )
    connect(base_node, surface, 'Base', base_output)
    connect(masks, surface, 'Masks')
    connect(outline, surface, 'Outline')
    connect(fresnel, surface, 'Fresnel')

    emission = node(material, u.MaterialExpressionCustom, 80, 520,
                    description='Gunner heat edge and early red silhouette',
                    output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
    emission_inputs = ['Masks', 'Intensity', 'Outline', 'Fresnel']
    if translucent:
        emission_inputs.insert(0, 'Base')
    emission.set_editor_property('inputs', [custom_input(name) for name in emission_inputs])
    emission.set_editor_property(
        'code',
        'float front=pow(saturate(Masks.y),1.45); '
        'float shell=saturate(Outline)*smoothstep(.68,.94,saturate(Fresnel)); '
        'return Intensity*(pow(saturate(Masks.x),1.45)*float3(1,.008,.018)*.090 '
        '+front*float3(1,.42,.58)*.34+shell*float3(1,.018,.055)*.10);',
    )
    if translucent:
        connect(base_node, emission, 'Base', base_output)
    connect(masks, emission, 'Masks')
    connect(intensity, emission, 'Intensity')
    connect(outline, emission, 'Outline')
    connect(fresnel, emission, 'Fresnel')

    final_emission = emission
    if original_emissive:
        final_emission = node(material, u.MaterialExpressionAdd, 330, 520)
        connect(original_emissive, final_emission, 'A', original_emissive_output)
        connect(emission, final_emission, 'B')

    remaining = node(material, u.MaterialExpressionComponentMask, -260, 1080,
                     r=False, g=False, b=True, a=False)
    connect(masks, remaining, '')
    held_mask = node(material, u.MaterialExpressionCustom, 0, 1020,
                     description='Keep Gunner outline after interior erosion',
                     output_type=u.CustomMaterialOutputType.CMOT_FLOAT1)
    held_mask.set_editor_property(
        'inputs', [custom_input(name) for name in ['Remaining', 'Outline', 'Fresnel']]
    )
    held_mask.set_editor_property(
        'code',
        'float shell=step(.78,saturate(Fresnel))*saturate(Outline); '
        'return max(Remaining,shell);',
    )
    connect(remaining, held_mask, 'Remaining')
    connect(outline, held_mask, 'Outline')
    connect(fresnel, held_mask, 'Fresnel')
    faded_mask = node(material, u.MaterialExpressionMultiply, 250, 1080)
    connect(held_mask, faded_mask, 'A')
    connect(opacity_fade, faded_mask, 'B')

    editing.connect_material_property(surface, '', u.MaterialProperty.MP_BASE_COLOR)
    editing.connect_material_property(final_emission, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.connect_material_property(
        faded_mask, '',
        u.MaterialProperty.MP_OPACITY if translucent else u.MaterialProperty.MP_OPACITY_MASK,
    )
    editing.recompile_material(material)
    library.save_loaded_asset(material)
    return material


bp = library.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)
masked = build_adapter(SOURCE, MASKED, False, True)
fade = build_adapter(SOURCE, FADE, True, True)
build_adapter(AK_METAL_SOURCE, AK_METAL, False, False)
build_adapter(AK_POLYMER_SOURCE, AK_POLYMER, False, False)

BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
cdo.set_editor_property('RadiantSkeletalFallbackMaterial', masked)
if 'RadiantSkeletalFadeMaterial' in BP.list_variables(bp):
    cdo.set_editor_property('RadiantSkeletalFadeMaterial', fade)
BP.compile_blueprint(bp, warnings_as_errors=True)
library.save_loaded_asset(bp)

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() != 'BP_RadiantDissolve_Test_C':
        continue
    actor.set_editor_property('RadiantSkeletalFallbackMaterial', masked)
    if 'RadiantSkeletalFadeMaterial' in BP.list_variables(bp):
        actor.set_editor_property('RadiantSkeletalFadeMaterial', fade)

print('RADIANT_GUNNER_ADAPTERS_REBUILT_FROM_SOURCE')
