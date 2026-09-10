"""Preserve source colour and finish every visible mesh in the target hierarchy."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
E = u.EditorAssetLibrary
M = u.MaterialEditingLibrary
BP_PATH = ROOT + '/BP_RadiantDissolve_Test'

GUNNER_SOURCE = '/Game/MineLearning/Characters/Gunner/Materials/M_Gunner'
GUNNER_ADAPTER = ROOT + '/M_RadiantDissolve_GunnerSurface'
AK_METAL_SOURCE = '/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Metal'
AK_METAL_ADAPTER = ROOT + '/M_RadiantDissolve_AKMetalSurface'
AK_POLYMER_SOURCE = '/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Polymer'
AK_POLYMER_ADAPTER = ROOT + '/M_RadiantDissolve_AKPolymerSurface'
AK_MAGAZINE_SOURCE = '/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Magazine'
AK_MAGAZINE_ADAPTER = ROOT + '/M_RadiantDissolve_AKMagazineSurface'
FALLBACK = ROOT + '/M_RadiantDissolve_Surface'


def custom_input(name):
    value = u.CustomInput()
    value.set_editor_property('input_name', name)
    return value


def expression(material, expression_class, x, y, **properties):
    result = M.create_material_expression(material, expression_class, x, y)
    for name, value in properties.items():
        result.set_editor_property(name, value)
    return result


def scalar(material, name, default, x, y):
    return expression(
        material,
        u.MaterialExpressionScalarParameter,
        x,
        y,
        parameter_name=name,
        default_value=default,
        group='Radiant Dissolve',
    )


def connect(source, destination, input_name, output_name=''):
    if not M.connect_material_expressions(source, output_name, destination, input_name):
        raise RuntimeError(
            f'Could not connect {source.get_name()} to '
            f'{destination.get_name()}.{input_name}'
        )


def set_usage(material, skeletal):
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_MASKED)
    material.set_editor_property('opacity_mask_clip_value', 0.20)
    material.set_editor_property('used_with_skeletal_mesh', skeletal)
    material.set_editor_property('automatically_set_usage_in_editor', True)
    if not skeletal:
        try:
            material.set_editor_property('used_with_nanite', True)
        except Exception:
            pass


def build_adapter(source_path, destination_path, skeletal=False):
    material = E.load_asset(destination_path)
    if material:
        set_usage(material, skeletal)
        return material
    material = E.duplicate_asset(source_path, destination_path)
    if not material:
        raise RuntimeError(f'Could not duplicate {source_path}')
    set_usage(material, skeletal)

    base = M.get_material_property_input_node(material, u.MaterialProperty.MP_BASE_COLOR)
    base_output = M.get_material_property_input_node_output_name(
        material, u.MaterialProperty.MP_BASE_COLOR
    )
    old_emissive = M.get_material_property_input_node(
        material, u.MaterialProperty.MP_EMISSIVE_COLOR
    )
    old_emissive_output = M.get_material_property_input_node_output_name(
        material, u.MaterialProperty.MP_EMISSIVE_COLOR
    )
    if not base:
        raise RuntimeError(f'{source_path} has no Base Color graph')

    masks = expression(
        material,
        u.MaterialExpressionMaterialFunctionCall,
        -520,
        520,
        material_function=E.load_asset(ROOT + '/MF_RadiantDissolve'),
    )
    outline = scalar(material, 'OutlineAmount', 0.0, -520, 720)
    opacity = scalar(material, 'OpacityFade', 1.0, -520, 850)
    intensity = scalar(material, 'HeatIntensity', 70.0, -520, 980)
    fresnel = expression(material, u.MaterialExpressionFresnel, -280, 760, exponent=1.35)

    surface = expression(
        material,
        u.MaterialExpressionCustom,
        80,
        300,
        description='Preserve source colour while heating',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
    )
    surface.set_editor_property(
        'inputs', [custom_input(name) for name in ['Base', 'Masks', 'Outline', 'Fresnel']]
    )
    surface.set_editor_property(
        'code',
        'float heat=saturate(Masks.x); '
        'float3 hot=Base+heat*float3(.055,.003,.006); '
        'float shell=saturate(Outline)*smoothstep(.70,.96,saturate(Fresnel)); '
        'return lerp(hot,hot+float3(.20,.006,.018),shell*.50);',
    )
    connect(base, surface, 'Base', base_output)
    connect(masks, surface, 'Masks')
    connect(outline, surface, 'Outline')
    connect(fresnel, surface, 'Fresnel')

    emission = expression(
        material,
        u.MaterialExpressionCustom,
        80,
        520,
        description='Radiant heat edge and silhouette',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
    )
    emission.set_editor_property(
        'inputs', [custom_input(name) for name in ['Masks', 'Intensity', 'Outline', 'Fresnel']]
    )
    emission.set_editor_property(
        'code',
        'float front=pow(saturate(Masks.y),1.45); '
        'float shell=saturate(Outline)*smoothstep(.68,.94,saturate(Fresnel)); '
        'return Intensity*(pow(saturate(Masks.x),1.45)*float3(1,.008,.018)*.090 '
        '+front*float3(1,.42,.58)*.34+shell*float3(1,.018,.055)*.10);',
    )
    connect(masks, emission, 'Masks')
    connect(intensity, emission, 'Intensity')
    connect(outline, emission, 'Outline')
    connect(fresnel, emission, 'Fresnel')

    final_emission = emission
    if old_emissive:
        final_emission = expression(material, u.MaterialExpressionAdd, 330, 520)
        connect(old_emissive, final_emission, 'A', old_emissive_output)
        connect(emission, final_emission, 'B')

    remaining = expression(
        material, u.MaterialExpressionComponentMask, -260, 1080,
        r=False, g=False, b=True, a=False,
    )
    connect(masks, remaining, '')
    held = expression(
        material,
        u.MaterialExpressionCustom,
        0,
        1020,
        description='Keep a readable outline after erosion',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
    )
    held.set_editor_property(
        'inputs', [custom_input(name) for name in ['Remaining', 'Outline', 'Fresnel']]
    )
    held.set_editor_property(
        'code',
        'float shell=step(.78,saturate(Fresnel))*saturate(Outline); '
        'return max(Remaining,shell);',
    )
    connect(remaining, held, 'Remaining')
    connect(outline, held, 'Outline')
    connect(fresnel, held, 'Fresnel')
    faded = expression(material, u.MaterialExpressionMultiply, 250, 1080)
    connect(held, faded, 'A')
    connect(opacity, faded, 'B')

    M.connect_material_property(surface, '', u.MaterialProperty.MP_BASE_COLOR)
    M.connect_material_property(final_emission, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    M.connect_material_property(faded, '', u.MaterialProperty.MP_OPACITY_MASK)
    M.recompile_material(material)
    E.save_loaded_asset(material)
    return material


adapters = [
    build_adapter(GUNNER_SOURCE, GUNNER_ADAPTER, True),
    build_adapter(AK_METAL_SOURCE, AK_METAL_ADAPTER),
    build_adapter(AK_POLYMER_SOURCE, AK_POLYMER_ADAPTER),
    build_adapter(AK_MAGAZINE_SOURCE, AK_MAGAZINE_ADAPTER),
]

# Existing adapters used a dark heat tint in BaseColor. Keep the texture and
# material colour intact; heat and the silhouette remain in Emissive.
for material in adapters:
    for node in M.get_material_expressions(material):
        if not isinstance(node, u.MaterialExpressionCustom):
            continue
        if node.get_editor_property('description') in {
            'Preserve Gunner surface while heating',
            'Preserve source colour while heating',
        }:
            node.set_editor_property('description', 'Preserve source colour while heating')
            node.set_editor_property(
                'code',
                'float heat=saturate(Masks.x); '
                'float3 hot=Base+heat*float3(.055,.003,.006); '
                'float shell=saturate(Outline)*smoothstep(.70,.96,saturate(Fresnel)); '
                'return lerp(hot,hot+float3(.20,.006,.018),shell*.50);',
            )
    M.recompile_material(material)
    E.save_loaded_asset(material)

fallback = E.load_asset(FALLBACK)
set_usage(fallback, True)
try:
    fallback.set_editor_property('used_with_nanite', True)
except Exception:
    pass
for node in M.get_material_expressions(fallback):
    if not isinstance(node, u.MaterialExpressionCustom):
        continue
    if node.get_editor_property('description') == 'Heat surface plus held cartoon silhouette':
        node.set_editor_property(
            'code',
            'float3 heated=Base+Masks.x*float3(.055,.003,.006); '
            'float shell=saturate(Outline)*smoothstep(.34,.82,saturate(Fresnel)); '
            'float hot=saturate(Masks.y); '
            'return lerp(heated,heated+float3(.20,.006,.018),shell*.50) '
            '+hot*float3(.18,.006,.014);',
        )
M.recompile_material(fallback)
E.save_loaded_asset(fallback)


bp = E.load_asset(BP_PATH)
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)

function_names = {entry.name for entry in BP.list_functions(bp)}
if 'RadiantCollectActor' not in function_names:
    graph = BP.add_function_graph(bp, 'RadiantCollectActor')
    BP.add_object_function_param(graph, 'ActorToCollect', u.Actor.static_class(), True)
BP.compile_blueprint(bp, warnings_as_errors=True)

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantCollectActor'), r'''(fn RadiantCollectActor (ActorToCollect)
 (for component (Actor|GetComponentsByClass ActorToCollect "/Script/Engine.StaticMeshComponent")
  (bind mesh (Utilities|Casting|CastToStaticMeshComponent component))
  (Utilities|FlowControl|Branch (Rendering|IsVisible mesh)
   (:then (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantStaticMeshes) mesh))
   (:else (CallFunction|RadiantNoOp))))
 (for component (Actor|GetComponentsByClass ActorToCollect "/Script/Engine.SkeletalMeshComponent")
  (bind mesh (Utilities|Casting|CastToSkeletalMeshComponent component))
  (Utilities|FlowControl|Branch (Rendering|IsVisible mesh)
   (:then (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantSkeletalMeshes) mesh))
   (:else (CallFunction|RadiantNoOp)))))''')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantResolve'), r'''(fn RadiantResolve ()
 (Utilities|Array|Clear (Variables|RadiantInternal|GetRadiantStaticMeshes))
 (Utilities|Array|Clear (Variables|RadiantInternal|GetRadiantSkeletalMeshes))
 (Utilities|IsValid (Variables|RadiantInternal|GetRadiantTarget)
  (:"Is Valid"
   (Rendering|SetVisibility (Variables|Default|GetTargetMesh))
   (CallFunction|RadiantCollectActor :ActorToCollect (Variables|RadiantInternal|GetRadiantTarget))
   (bind attachedActors (Actor|GetAttachedActors (Variables|RadiantInternal|GetRadiantTarget) true true))
   (for actor attachedActors
    (CallFunction|RadiantCollectActor :ActorToCollect actor)))
  (:"Is Not Valid"
   (Rendering|SetVisibility (Variables|Default|GetTargetMesh) true)
   (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantStaticMeshes) (Variables|Default|GetTargetMesh)))))''')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantPrepareStatic'), r'''(fn RadiantPrepareStatic ()
 (for mesh (Variables|RadiantInternal|GetRadiantStaticMeshes)
  (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantStaticMaterialCounts) (Rendering|Material|GetNumMaterials mesh))
  (for slot (range (Rendering|Material|GetNumMaterials mesh))
   (bind source (Rendering|Material|GetMaterial mesh slot))
   (bind sourceBase (Rendering|Material|GetBaseMaterial source))
   (bind sourceName (Utilities|GetObjectName sourceBase))
   (bind isMetal (Utilities|String|EqualExactly(String) sourceName "M_AK_Metal"))
   (bind isPolymer (Utilities|String|EqualExactly(String) sourceName "M_AK_Polymer"))
   (bind isMagazine (Utilities|String|EqualExactly(String) sourceName "M_AK_Magazine"))
   (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantOriginalStaticMaterials) source)
   (Utilities|FlowControl|Branch isMetal
    (:then
     (bind metalMid (Rendering|Material|CreateDynamicMaterialInstance mesh slot "/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_AKMetalSurface.M_RadiantDissolve_AKMetalSurface"))
     (Rendering|Material|CopyMaterialInstanceParameters metalMid source true))
    (:else
     (Utilities|FlowControl|Branch isPolymer
      (:then
       (bind polymerMid (Rendering|Material|CreateDynamicMaterialInstance mesh slot "/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_AKPolymerSurface.M_RadiantDissolve_AKPolymerSurface"))
       (Rendering|Material|CopyMaterialInstanceParameters polymerMid source true))
      (:else
       (Utilities|FlowControl|Branch isMagazine
        (:then
         (bind magazineMid (Rendering|Material|CreateDynamicMaterialInstance mesh slot "/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_AKMagazineSurface.M_RadiantDissolve_AKMagazineSurface"))
         (Rendering|Material|CopyMaterialInstanceParameters magazineMid source true))
        (:else
         (bind fallbackMid (Rendering|Material|CreateDynamicMaterialInstance mesh slot "/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_Surface.M_RadiantDissolve_Surface"))
         (Rendering|Material|CopyMaterialInstanceParameters fallbackMid source true))))))))))''')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantPrepareSkeletal'), r'''(fn RadiantPrepareSkeletal ()
 (for mesh (Variables|RadiantInternal|GetRadiantSkeletalMeshes)
  (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantSkeletalMaterialCounts) (Rendering|Material|GetNumMaterials mesh))
  (for slot (range (Rendering|Material|GetNumMaterials mesh))
   (bind source (Rendering|Material|GetMaterial mesh slot))
   (bind sourceBase (Rendering|Material|GetBaseMaterial source))
   (bind isGunner (Utilities|String|EqualExactly(String) (Utilities|GetObjectName sourceBase) "M_Gunner"))
   (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantOriginalSkeletalMaterials) source)
   (Utilities|FlowControl|Branch isGunner
    (:then
     (bind gunnerMid (Rendering|Material|CreateDynamicMaterialInstance mesh slot "/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_GunnerSurface.M_RadiantDissolve_GunnerSurface"))
     (Rendering|Material|CopyMaterialInstanceParameters gunnerMid source true))
    (:else
     (bind skeletalFallbackMid (Rendering|Material|CreateDynamicMaterialInstance mesh slot "/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_Surface.M_RadiantDissolve_Surface"))
     (Rendering|Material|CopyMaterialInstanceParameters skeletalFallbackMid source true))))))''')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantReset'), r'''(fn RadiantReset ()
 (Utilities|Time|ClearTimerbyFunctionName self "RadiantUpdate")
 (for fx (Variables|RadiantInternal|GetRadiantActiveParticles)
  (Components|DestroyComponent fx))
 (Utilities|Array|Clear (Variables|RadiantInternal|GetRadiantActiveParticles))
 (for mesh (Variables|RadiantInternal|GetRadiantStaticMeshes)
  (Rendering|SetVisibility mesh true false))
 (for mesh (Variables|RadiantInternal|GetRadiantSkeletalMeshes)
  (Rendering|SetVisibility mesh true false))
 (CallFunction|RadiantRestore)
 (CallFunction|RadiantResolve)
 (CallFunction|RadiantPlaceOrigin)
 (CallFunction|RadiantComputeCoverage)
 (Variables|RadiantInternal|SetRadiantElapsed 0)
 (Variables|RadiantInternal|SetRadiantPreviewTime 0)
 (Variables|RadiantInternal|SetRadiantPreviewEnabled false)
 (CallFunction|RadiantApplyFrame))''')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantFinish'), r'''(fn RadiantFinish ()
 (Utilities|Time|ClearTimerbyFunctionName self "RadiantUpdate")
 (for mesh (Variables|RadiantInternal|GetRadiantStaticMeshes)
  (Rendering|SetVisibility mesh false false))
 (for mesh (Variables|RadiantInternal|GetRadiantSkeletalMeshes)
  (Rendering|SetVisibility mesh false false))
 (for fx (Variables|RadiantInternal|GetRadiantActiveParticles)
  (Niagara|SetNiagaraVariable(Float) fx "User.SpawnRate" 0)
  (Components|Activation|Deactivate fx)))''')

# Keep source-specific compatibility assets internal. The caller only sees the
# unified Actor/origin API and visual controls.
for name in [
    'RadiantActors',
    'RadiantSkeletalFallbackMaterial',
    'RadiantAKMetalDissolveMaterial',
    'RadiantAKPolymerDissolveMaterial',
    'RadiantAKMetalSourceMaterial',
    'RadiantAKPolymerSourceMaterial',
]:
    if name in BP.list_variables(bp):
        BP.remove_variable(bp, name)

BP.compile_blueprint(bp, warnings_as_errors=True)
E.save_loaded_asset(bp)

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() == 'BP_RadiantDissolve_Test_C':
        actor.call_method('RadiantReset')

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_SOURCE_COLOR_AND_COMPLETE_MESHES_FIXED')
