"""Radiant dissolve V2: generic material fallback, skeletal targets, held outline and origin marker."""
import unreal as u
from editor_toolset.toolsets.actor import ActorTools as AT
from editor_toolset.toolsets.blueprint import BlueprintTools as BP, ContainerType

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
E = u.EditorAssetLibrary
M = u.MaterialEditingLibrary


def custom_input(name):
    value = u.CustomInput()
    value.set_editor_property('input_name', name)
    return value


def expression(material, expression_class, x, y, **properties):
    node = M.create_material_expression(material, expression_class, x, y)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def scalar(material, name, default, x, y, group='Radiant Dissolve'):
    return expression(
        material,
        u.MaterialExpressionScalarParameter,
        x,
        y,
        parameter_name=name,
        default_value=default,
        group=group,
    )


def connect(source, destination, input_name, output_name=''):
    if not M.connect_material_expressions(source, output_name, destination, input_name):
        raise RuntimeError(f'Could not connect {source.get_name()} -> {destination.get_name()}.{input_name}')


def rebuild_dissolve_material(material, base_texture=None):
    M.delete_all_material_expressions(material)
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_MASKED)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('used_with_skeletal_mesh', True)

    function = E.load_asset(ROOT + '/MF_RadiantDissolve')
    function_call = expression(
        material,
        u.MaterialExpressionMaterialFunctionCall,
        -620,
        0,
        material_function=function,
    )
    if base_texture:
        base = expression(
            material,
            u.MaterialExpressionTextureSampleParameter2D,
            -620,
            -430,
            parameter_name='SurfaceBaseColor',
            texture=base_texture,
            group='Surface',
        )
        base_output = 'RGB'
    else:
        base = expression(
            material,
            u.MaterialExpressionVectorParameter,
            -620,
            -430,
            parameter_name='BaseColor',
            default_value=u.LinearColor(0.20, 0.23, 0.27, 1.0),
            group='Surface',
        )
        base_output = 'RGB'

    outline = scalar(material, 'OutlineAmount', 0.0, -620, 290)
    opacity_fade = scalar(material, 'OpacityFade', 1.0, -620, 410)
    intensity = scalar(material, 'HeatIntensity', 70.0, -620, 530)
    fresnel = expression(material, u.MaterialExpressionFresnel, -360, 330)

    surface = expression(
        material,
        u.MaterialExpressionCustom,
        -80,
        -220,
        description='Heat surface plus held cartoon silhouette',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
    )
    surface.set_editor_property('inputs', [custom_input(name) for name in ['Base', 'Masks', 'Outline', 'Fresnel']])
    surface.set_editor_property(
        'code',
        'float3 heated=lerp(Base,float3(.25,.004,.012),Masks.x*.9); '
        'float shell=saturate(Outline)*pow(saturate(Fresnel),.65); '
        'float hot=saturate(Masks.y); '
        'return lerp(heated,float3(.38,.006,.02),shell*.78)+hot*float3(.55,.018,.035);',
    )
    connect(base, surface, 'Base', base_output)
    connect(function_call, surface, 'Masks')
    connect(outline, surface, 'Outline')
    connect(fresnel, surface, 'Fresnel')

    emission = expression(
        material,
        u.MaterialExpressionCustom,
        -80,
        80,
        description='Red heat, white-pink front and lingering silhouette',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
    )
    emission.set_editor_property('inputs', [custom_input(name) for name in ['Masks', 'Intensity', 'Outline', 'Fresnel']])
    emission.set_editor_property(
        'code',
        'float front=pow(saturate(Masks.y),1.45); '
        'float shell=saturate(Outline)*pow(saturate(Fresnel),.65); '
        'return Intensity*(pow(saturate(Masks.x),1.45)*float3(1,.008,.018)*.34 '
        '+front*float3(1,.42,.58)*2.15+shell*float3(1,.075,.14)*.82);',
    )
    connect(function_call, emission, 'Masks')
    connect(intensity, emission, 'Intensity')
    connect(outline, emission, 'Outline')
    connect(fresnel, emission, 'Fresnel')

    remaining = expression(material, u.MaterialExpressionComponentMask, -300, 610, r=False, g=False, b=True, a=False)
    connect(function_call, remaining, '')
    blue_noise_class = u.load_class(None, '/Script/Engine.MaterialExpressionScalarBlueNoise')
    if not blue_noise_class:
        raise RuntimeError('MaterialExpressionScalarBlueNoise class is unavailable')
    blue_noise = expression(material, blue_noise_class, -300, 760)
    fade_dither = expression(
        material,
        u.MaterialExpressionCustom,
        -60,
        660,
        description='Blue-noise silhouette fade',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
    )
    fade_dither.set_editor_property('inputs', [custom_input('Fade'), custom_input('Noise')])
    fade_dither.set_editor_property('code', 'return step(Noise,Fade);')
    connect(opacity_fade, fade_dither, 'Fade')
    connect(blue_noise, fade_dither, 'Noise')
    held_mask = expression(
        material,
        u.MaterialExpressionCustom,
        -60,
        560,
        description='Keep a readable Fresnel silhouette after the interior dissolves',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
    )
    held_mask.set_editor_property('inputs', [custom_input(name) for name in ['Remaining', 'Outline', 'Fresnel']])
    held_mask.set_editor_property(
        'code',
        'float shell=step(.48,saturate(Fresnel))*saturate(Outline); return max(Remaining,shell);',
    )
    connect(remaining, held_mask, 'Remaining')
    connect(outline, held_mask, 'Outline')
    connect(fresnel, held_mask, 'Fresnel')
    faded = expression(material, u.MaterialExpressionMultiply, 190, 610)
    connect(held_mask, faded, 'A')
    connect(fade_dither, faded, 'B')

    roughness = scalar(material, 'Roughness', 0.68, -80, 760, 'Surface')
    M.connect_material_property(surface, '', u.MaterialProperty.MP_BASE_COLOR)
    M.connect_material_property(emission, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    M.connect_material_property(faded, '', u.MaterialProperty.MP_OPACITY_MASK)
    M.connect_material_property(roughness, '', u.MaterialProperty.MP_ROUGHNESS)
    M.recompile_material(material)
    E.save_loaded_asset(material)


ore_texture = E.load_asset('/Game/MineLearning/Mining/Ores/Iron/Textures/T_Ore_Iron_100_BaseColor')
rebuild_dissolve_material(E.load_asset(ROOT + '/M_RadiantDissolve_Test'), ore_texture)
rebuild_dissolve_material(E.load_asset(ROOT + '/M_RadiantDissolve_Surface'))

material_instance = E.load_asset(ROOT + '/MI_RadiantDissolve_Test')
M.set_material_instance_parent(material_instance, E.load_asset(ROOT + '/M_RadiantDissolve_Test'))
E.save_loaded_asset(material_instance)


# Q-shaped white motes: a readable round core with a soft edge.
mote = E.load_asset(ROOT + '/M_RadiantMote')
M.delete_all_material_expressions(mote)
mote.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
mote.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
mote.set_editor_property('used_with_niagara_sprites', True)
uv = expression(mote, u.MaterialExpressionTextureCoordinate, -620, 0)
particle_color = expression(mote, u.MaterialExpressionParticleColor, -620, 220)
color = expression(
    mote,
    u.MaterialExpressionCustom,
    -280,
    0,
    description='Cartoon white energy pearl',
    output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
)
color.set_editor_property('inputs', [custom_input('UV')])
color.set_editor_property('code', 'float r=length(UV-.5)*2; float core=smoothstep(.78,.18,r); return float3(1,1,1)*(70+95*core);')
alpha = expression(
    mote,
    u.MaterialExpressionCustom,
    -280,
    230,
    description='Round soft edge multiplied by particle lifetime',
    output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
)
alpha.set_editor_property('inputs', [custom_input('UV'), custom_input('Alpha')])
alpha.set_editor_property('code', 'float r=length(UV-.5)*2; return smoothstep(1,.68,r)*Alpha;')
connect(uv, color, 'UV')
connect(uv, alpha, 'UV')
connect(particle_color, alpha, 'Alpha', 'A')
M.connect_material_property(color, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
M.connect_material_property(alpha, '', u.MaterialProperty.MP_OPACITY)
M.recompile_material(mote)
E.save_loaded_asset(mote)


# Editor-only-looking origin marker (the component is hidden in game by default).
marker = E.load_asset(ROOT + '/M_RadiantOriginMarker')
if not marker:
    marker = u.AssetToolsHelpers.get_asset_tools().create_asset('M_RadiantOriginMarker', ROOT, u.Material, u.MaterialFactoryNew())
M.delete_all_material_expressions(marker)
marker.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
marker.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
marker.set_editor_property('two_sided', True)
marker_color = expression(marker, u.MaterialExpressionConstant3Vector, -180, 0, constant=u.LinearColor(1.0, 0.03, 0.12, 1.0))
marker_power = expression(marker, u.MaterialExpressionMultiply, 40, 0)
marker_gain = expression(marker, u.MaterialExpressionConstant, -180, 150, r=18.0)
connect(marker_color, marker_power, 'A')
connect(marker_gain, marker_power, 'B')
marker_alpha = expression(marker, u.MaterialExpressionConstant, 40, 190, r=0.8)
M.connect_material_property(marker_power, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
M.connect_material_property(marker_alpha, '', u.MaterialProperty.MP_OPACITY)
M.recompile_material(marker)
E.save_loaded_asset(marker)


# Create the skeletal Niagara copy before the Blueprint references it.
skeletal_system_path = ROOT + '/NS_RadiantDissolve_Skeletal'
if not E.does_asset_exist(skeletal_system_path):
    E.duplicate_asset(ROOT + '/NS_RadiantDissolve_Test', skeletal_system_path)


bp = E.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)


def add_object_variable(name, cls, array=False):
    if name not in BP.list_variables(bp):
        BP.add_object_variable(
            bp,
            name,
            cls.static_class(),
            container_type=ContainerType.ARRAY if array else None,
        )


def add_value_variable(name, type_name):
    if name not in BP.list_variables(bp):
        BP.add_variable(bp, name, type_name)


add_object_variable('BoundSkeletalMeshes', u.SkeletalMeshComponent, True)
add_object_variable('OriginalSkeletalMaterials', u.MaterialInterface, True)
add_value_variable('SkeletalMaterialCounts', 'int')
# Correct the container if this is a newly-added value variable.
if 'SkeletalMaterialCounts' in BP.list_variables(bp):
    # remove/re-add is intentionally avoided for existing assets; a later check verifies it is an array.
    pass
add_object_variable('FallbackMaterial', u.MaterialInterface)
for name in [
    'AutoPlaceOrigin',
    'OriginHeightRatio',
    'ParticleFrontWidth',
    'DissolveStartFraction',
    'OutlineHoldStartFraction',
    'OutlineFadeStartFraction',
    'MeshGoneFraction',
]:
    add_value_variable(name, 'bool' if name == 'AutoPlaceOrigin' else 'float')

# SkeletalMaterialCounts must be an array. Recreate only when this script added it as a scalar.
if 'SkeletalMaterialCounts' in BP.list_variables(bp):
    try:
        BP.remove_variable(bp, 'SkeletalMaterialCounts')
    except Exception:
        pass
    BP.add_variable(bp, 'SkeletalMaterialCounts', 'int', container_type=ContainerType.ARRAY)

for name in [
    'TargetActor', 'FallbackMaterial', 'AutoPlaceOrigin', 'OriginHeightRatio',
    'Duration', 'PropagationDistance', 'NoiseStrength', 'HeatIntensity',
    'DissolveEdgeWidth', 'ParticleSampleRate', 'ParticleFrontWidth',
    'DissolveStartFraction', 'OutlineHoldStartFraction',
    'OutlineFadeStartFraction', 'MeshGoneFraction', 'PreviewEnabled', 'PreviewTime',
]:
    BP.set_variable_instance_editable(bp, name, True)

for name in ['TargetActor', 'FallbackMaterial', 'AutoPlaceOrigin', 'OriginHeightRatio']:
    BP.set_variable_category(bp, name, 'Target')
for name in [
    'Duration', 'PropagationDistance', 'NoiseStrength', 'HeatIntensity',
    'DissolveEdgeWidth', 'DissolveStartFraction', 'OutlineHoldStartFraction',
    'OutlineFadeStartFraction', 'MeshGoneFraction',
]:
    BP.set_variable_category(bp, name, 'Dissolve')
for name in ['ParticleSampleRate', 'ParticleFrontWidth']:
    BP.set_variable_category(bp, name, 'Particles')
for name in ['PreviewEnabled', 'PreviewTime']:
    BP.set_variable_category(bp, name, 'EditorPreview')

if not BP.get_graph(bp, 'UpdateAutoOrigin'):
    BP.add_function_graph(bp, 'UpdateAutoOrigin')

BP.compile_blueprint(bp)

components = AT.get_components(BP.get_default_object(bp))
origin = next(component for component in components if component.get_name().startswith('DissolveOrigin'))
origin_marker = next((component for component in components if component.get_name().startswith('OriginMarker')), None)
if not origin_marker:
    origin_marker = AT.add_component(bp, u.StaticMeshComponent.static_class(), 'OriginMarker')
    BP.compile_blueprint(bp)
    components = AT.get_components(BP.get_default_object(bp))
    origin = next(component for component in components if component.get_name().startswith('DissolveOrigin'))
    origin_marker = next(component for component in components if component.get_name().startswith('OriginMarker'))
AT.set_parent_component(origin_marker, origin)
origin_marker.set_static_mesh(E.load_asset('/Engine/BasicShapes/Sphere'))
origin_marker.set_material(0, marker)
origin_marker.set_editor_property('relative_location', u.Vector(0, 0, 0))
origin_marker.set_editor_property('relative_scale3d', u.Vector(0.065, 0.065, 0.065))
origin_marker.set_editor_property('hidden_in_game', True)
origin_marker.set_editor_property('cast_shadow', False)
origin_marker.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)

# Component/variable edits reinstate the generated class. Reload before resolving
# localized variable node identifiers for the graph writer.
bp = E.load_asset(ROOT + '/BP_RadiantDissolve_Test')

for graph_name in [
    'ResolveTargets', 'Initialize', 'UpdateAutoOrigin', 'PrepareMaterials',
    'RestoreMaterials', 'SpawnParticles', 'ApplyFrame', 'Reset', 'TestDissolve',
    'FinishDissolve', 'UpdateDissolve', 'UserConstructionScript',
]:
    if not BP.get_graph(bp, graph_name):
        BP.add_function_graph(bp, graph_name)


def write_graph(name, code):
    BP.write_graph_dsl(BP.get_graph(bp, name), code)
    print('RADIANT_V2_GRAPH', name)


write_graph('ResolveTargets', '''(fn ResolveTargets ()
 (Utilities|Array|Clear (变量|Default|GetBoundMeshes))
 (Utilities|Array|Clear (变量|Default|GetBoundSkeletalMeshes))
 (Utilities|IsValid (变量|Target|GetTargetActor)
  (:"Is Valid"
   (Rendering|SetVisibility :self (变量|Default|GetTargetMesh) :bNewVisibility false)
   (for component (Actor|GetComponentsByClass :self (变量|Target|GetTargetActor) :ComponentClass "/Script/Engine.StaticMeshComponent")
    (bind mesh (工具|Casting|CastToStaticMeshComponent :Object component)
     (:then (Utilities|Array|Add (变量|Default|GetBoundMeshes) mesh))
     (:CastFailed)))
   (for component (Actor|GetComponentsByClass :self (变量|Target|GetTargetActor) :ComponentClass "/Script/Engine.SkeletalMeshComponent")
    (bind mesh (工具|Casting|CastToSkeletalMeshComponent :Object component)
     (:then (Utilities|Array|Add (变量|Default|GetBoundSkeletalMeshes) mesh))
     (:CastFailed))))
  (:"Is Not Valid"
   (Rendering|SetVisibility :self (变量|Default|GetTargetMesh) :bNewVisibility true)
   (Utilities|Array|Add (变量|Default|GetBoundMeshes) (变量|Default|GetTargetMesh)))))''')

write_graph('UpdateAutoOrigin', '''(fn UpdateAutoOrigin ()
 (工具|流程控制|Branch (变量|Target|GetAutoPlaceOrigin)
  (:then
   (Utilities|IsValid (变量|Target|GetTargetActor)
    (:"Is Valid"
     (bind (center extent) (Collision|GetActorBounds :self (变量|Target|GetTargetActor) :bOnlyCollidingComponents false :bIncludeFromChildActors true))
     (bind height (* (Math|Vector|VectorLength extent) (变量|Target|GetOriginHeightRatio)))
     (Transformation|SetWorldLocation :self (变量|Default|GetDissolveOrigin) :NewLocation (+ center (Math|Vector|MakeVector :X 0 :Y 0 :Z height)) :bSweep false :bTeleport true))
    (:"Is Not Valid"
     (bind (center extent radius) (Collision|GetComponentBounds (变量|Default|GetTargetMesh)))
     (bind height (* (Math|Vector|VectorLength extent) (变量|Target|GetOriginHeightRatio)))
     (Transformation|SetWorldLocation :self (变量|Default|GetDissolveOrigin) :NewLocation (+ center (Math|Vector|MakeVector :X 0 :Y 0 :Z height)) :bSweep false :bTeleport true)))
  (:else))))''')

write_graph('Initialize', '''(fn Initialize ()
 (CallFunction|ResolveTargets)
 (CallFunction|UpdateAutoOrigin)
 (变量|Default|SetEffectiveRadius 0)
 (bind origin (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
 (for mesh (变量|Default|GetBoundMeshes)
  (bind (center extent radius) (Collision|GetComponentBounds mesh))
  (bind reach (+ (+ (Math|Vector|Distance(Vector) center origin) (Math|Vector|VectorLength extent)) 2))
  (变量|Default|SetEffectiveRadius (工具|Select (变量|Default|GetEffectiveRadius) reach (> reach (变量|Default|GetEffectiveRadius))))
 (for mesh (变量|Default|GetBoundSkeletalMeshes)
  (bind (center extent radius) (Collision|GetComponentBounds mesh))
  (bind reach (+ (+ (Math|Vector|Distance(Vector) center origin) (Math|Vector|VectorLength extent)) 2))
  (变量|Default|SetEffectiveRadius (工具|Select (变量|Default|GetEffectiveRadius) reach (> reach (变量|Default|GetEffectiveRadius)))))))''')

prepare_loop_static = '''
 (for mesh (变量|Default|GetBoundMeshes)
  (Utilities|Array|Add (变量|Default|GetMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (Utilities|Array|Add (变量|Default|GetOriginalMaterials) (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (bind fallbackMid (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|Target|GetFallbackMaterial)))
   (变量|Default|SetDynamicMaterial fallbackMid)))'''
prepare_loop_skeletal = '''
 (for mesh (变量|Default|GetBoundSkeletalMeshes)
  (Utilities|Array|Add (变量|Default|GetSkeletalMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (Utilities|Array|Add (变量|Default|GetOriginalSkeletalMaterials) (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (bind fallbackMid (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|Target|GetFallbackMaterial)))
   (变量|Default|SetDynamicMaterial fallbackMid)))'''
write_graph('PrepareMaterials', '(fn PrepareMaterials ()\n' + prepare_loop_static + prepare_loop_skeletal + ')')

write_graph('RestoreMaterials', '''(fn RestoreMaterials ()
 (变量|Default|SetMaterialCursor 0)
 (for meshIndex (range (Utilities|Array|Length (变量|Default|GetMaterialCounts)))
  (bind mesh (Utilities|Array|Get(acopy) (变量|Default|GetBoundMeshes) meshIndex))
  (for slot (range (Utilities|Array|Get(acopy) (变量|Default|GetMaterialCounts) meshIndex))
   (Rendering|Material|SetMaterial :self mesh :ElementIndex slot :Material (Utilities|Array|Get(acopy) (变量|Default|GetOriginalMaterials) (变量|Default|GetMaterialCursor)))
   (变量|Default|SetMaterialCursor (+ (变量|Default|GetMaterialCursor) 1))))
 (变量|Default|SetMaterialCursor 0)
 (for meshIndex (range (Utilities|Array|Length (变量|Default|GetSkeletalMaterialCounts)))
  (bind mesh (Utilities|Array|Get(acopy) (变量|Default|GetBoundSkeletalMeshes) meshIndex))
  (for slot (range (Utilities|Array|Get(acopy) (变量|Default|GetSkeletalMaterialCounts) meshIndex))
   (Rendering|Material|SetMaterial :self mesh :ElementIndex slot :Material (Utilities|Array|Get(acopy) (变量|Default|GetOriginalSkeletalMaterials) (变量|Default|GetMaterialCursor)))
   (变量|Default|SetMaterialCursor (+ (变量|Default|GetMaterialCursor) 1))))
 (Utilities|Array|Clear (变量|Default|GetOriginalMaterials))
 (Utilities|Array|Clear (变量|Default|GetMaterialCounts))
 (Utilities|Array|Clear (变量|Default|GetOriginalSkeletalMaterials))
 (Utilities|Array|Clear (变量|Default|GetSkeletalMaterialCounts)))''')

write_graph('SpawnParticles', '''(fn SpawnParticles ()
 (for mesh (变量|Default|GetBoundMeshes)
  (bind fx (Niagara|SpawnSystemAttached :SystemTemplate "/Game/MineLearning/VFX/RadiantDissolve/NS_RadiantDissolve_Test.NS_RadiantDissolve_Test" :AttachToComponent mesh :bAutoDestroy false :bAutoActivate false :bPreCullCheck false))
  (Niagara|SetNiagaraStaticMeshComponent :NiagaraSystem fx :OverrideName "User.TargetMesh" :StaticMeshComponent mesh)
  (Utilities|Array|Add (变量|Default|GetActiveParticles) fx))
 (for mesh (变量|Default|GetBoundSkeletalMeshes)
  (bind fx (Niagara|SpawnSystemAttached :SystemTemplate "/Game/MineLearning/VFX/RadiantDissolve/NS_RadiantDissolve_Skeletal.NS_RadiantDissolve_Skeletal" :AttachToComponent mesh :bAutoDestroy false :bAutoActivate false :bPreCullCheck false))
  (Niagara|SetNiagaraSkeletalMeshComponent :NiagaraSystem fx :OverrideName "User.TargetSkeletalMesh" :SkeletalMeshComponent mesh)
  (Utilities|Array|Add (变量|Default|GetActiveParticles) fx))
 (CallFunction|ApplyFrame)
 (for fx (变量|Default|GetActiveParticles)
  (Components|Activation|Activate :self fx :bReset true)))''')

apply_mesh_static = '''
 (for mesh (变量|Default|GetBoundMeshes)
  (Rendering|Material|SetVectorParameterValueOnMaterials :self mesh :ParameterName "DissolveOriginWS" :ParameterValue origin)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveProgress" :ParameterValue t)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatRadius" :ParameterValue heat)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveRadius" :ParameterValue dissolve)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "NoiseStrength" :ParameterValue noise)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatIntensity" :ParameterValue (变量|Dissolve|GetHeatIntensity))
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveEdgeWidth" :ParameterValue (变量|Dissolve|GetDissolveEdgeWidth))
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OutlineAmount" :ParameterValue outline)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OpacityFade" :ParameterValue opacity))'''
apply_mesh_skeletal = apply_mesh_static.replace('GetBoundMeshes', 'GetBoundSkeletalMeshes')
write_graph('ApplyFrame', '''(fn ApplyFrame ()
 (bind duration (变量|Dissolve|GetDuration))
 (bind rawT (/ (变量|Default|GetElapsed) (工具|Select 0.1 duration (> duration 0.1))))
 (bind t (工具|Select (工具|Select rawT 1 (> rawT 1)) 0 (< rawT 0)))
 (bind reach (工具|Select (变量|Default|GetEffectiveRadius) (变量|Dissolve|GetPropagationDistance) (> (变量|Dissolve|GetPropagationDistance) 0)))
 (bind rawNoise (变量|Dissolve|GetNoiseStrength))
 (bind absNoise (工具|Select (- 0 rawNoise) rawNoise (> rawNoise 0)))
 (bind maxNoise (* reach 0.22))
 (bind noise (工具|Select absNoise maxNoise (> absNoise maxNoise)))
 (bind travel (+ reach noise))
 (bind start (变量|Dissolve|GetDissolveStartFraction))
 (bind hold (变量|Dissolve|GetOutlineHoldStartFraction))
 (bind fade (变量|Dissolve|GetOutlineFadeStartFraction))
 (bind gone (变量|Dissolve|GetMeshGoneFraction))
 (bind earlyRaw (/ (- t start) (工具|Select 0.01 (- hold start) (> (- hold start) 0.01))))
 (bind early (工具|Select (工具|Select earlyRaw 1 (> earlyRaw 1)) 0 (< earlyRaw 0)))
 (bind lateRaw (/ (- t fade) (工具|Select 0.01 (- gone fade) (> (- gone fade) 0.01))))
 (bind late (工具|Select (工具|Select lateRaw 1 (> lateRaw 1)) 0 (< lateRaw 0)))
 (bind front (工具|Select (+ 0.88 (* late 0.12)) (* early 0.88) (< t fade)))
 (bind heatRaw (/ t (工具|Select 0.01 hold (> hold 0.01))))
 (bind heatT (工具|Select (工具|Select heatRaw 0 (< heatRaw 0)) 1 (> heatRaw 1)))
 (bind heat (- (* heatT (+ travel 52)) 12))
 (bind dissolve (- (* front (+ travel 40)) 40))
 (bind opacity (工具|Select (- 1 late) 1 (< t fade)))
 (bind outlineRaw (/ (- t (- hold 0.07)) 0.07))
 (bind outlineIn (工具|Select (工具|Select outlineRaw 1 (> outlineRaw 1)) 0 (< outlineRaw 0)))
 (bind outline (* outlineIn opacity))
 (bind particleRaw (/ (- t 0.42) (工具|Select 0.01 (- fade 0.42) (> (- fade 0.42) 0.01))))
 (bind particleRamp (工具|Select (工具|Select particleRaw 1 (> particleRaw 1)) 0 (< particleRaw 0)))
 (bind componentCount (+ (Utilities|Array|Length (变量|Default|GetBoundMeshes)) (Utilities|Array|Length (变量|Default|GetBoundSkeletalMeshes))))
 (bind perMeshRate (/ (* (变量|Particles|GetParticleSampleRate) (+ 0.35 (* particleRamp 1.65))) (工具|Select 1 componentCount (> componentCount 0))))
 (bind frontWidth (* (变量|Particles|GetParticleFrontWidth) (+ 1 (* late 1.35))))
 (bind origin (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
''' + apply_mesh_static + apply_mesh_skeletal + '''
 (for fx (变量|Default|GetActiveParticles)
  (Niagara|SetNiagaraVariable(Position) :self fx :InVariableName "User.DissolveOriginWS" :InValue origin)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.DissolveRadius" :InValue dissolve)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.NoiseStrength" :InValue noise)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.FrontWidth" :InValue frontWidth)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.SpawnRate" :InValue perMeshRate)))''')

write_graph('Reset', '''(fn Reset ()
 (Utilities|Time|ClearTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "UpdateDissolve")
 (变量|Default|SetElapsed 0)
 (变量|EditorPreview|SetPreviewTime 0)
 (变量|EditorPreview|SetPreviewEnabled false)
 (for fx (变量|Default|GetActiveParticles)
  (Components|DestroyComponent :self fx))
 (Utilities|Array|Clear (变量|Default|GetActiveParticles))
 (CallFunction|RestoreMaterials)
 (CallFunction|Initialize)
 (CallFunction|ApplyFrame))''')

write_graph('TestDissolve', '''(fn TestDissolve ()
 (CallFunction|Reset)
 (bind componentCount (+ (Utilities|Array|Length (变量|Default|GetBoundMeshes)) (Utilities|Array|Length (变量|Default|GetBoundSkeletalMeshes))))
 (工具|流程控制|Branch (> componentCount 0)
  (:then
   (CallFunction|PrepareMaterials)
   (CallFunction|SpawnParticles)
   (变量|Default|SetStartTime (Utilities|Time|GetGameTimeinSeconds))
   (Utilities|Time|SetTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "UpdateDissolve" :Time 0.0166667 :bLooping true :bMaxOncePerFrame true))
  (:else
   (Development|PrintString "Radiant Dissolve: target has no StaticMeshComponent or SkeletalMeshComponent."))))''')

write_graph('FinishDissolve', '''(fn FinishDissolve ()
 (Utilities|Time|ClearTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "UpdateDissolve")
 (for fx (变量|Default|GetActiveParticles)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.SpawnRate" :InValue 0)
  (Components|Activation|Deactivate :self fx)))''')

write_graph('UpdateDissolve', '''(fn UpdateDissolve ()
 (变量|Default|SetElapsed (- (Utilities|Time|GetGameTimeinSeconds) (变量|Default|GetStartTime)))
 (CallFunction|ApplyFrame)
 (工具|流程控制|Branch (>= (变量|Default|GetElapsed) (变量|Dissolve|GetDuration))
  (:then (CallFunction|FinishDissolve))
  (:else)))''')

write_graph('UserConstructionScript', '''(fn ConstructionScript ()
 (CallFunction|Initialize)
 (变量|Default|SetElapsed (工具|Select 0 (变量|EditorPreview|GetPreviewTime) (变量|EditorPreview|GetPreviewEnabled)))
 (CallFunction|ApplyFrame))''')

BP.compile_blueprint(bp)
cdo = BP.get_default_object(bp)
defaults = {
    'FallbackMaterial': E.load_asset(ROOT + '/M_RadiantDissolve_Surface'),
    'AutoPlaceOrigin': True,
    'OriginHeightRatio': 0.22,
    'Duration': 2.4,
    'PropagationDistance': 0.0,
    'NoiseStrength': 12.0,
    'HeatIntensity': 88.0,
    'DissolveEdgeWidth': 4.0,
    'ParticleSampleRate': 2600.0,
    'ParticleFrontWidth': 7.0,
    'DissolveStartFraction': 0.12,
    'OutlineHoldStartFraction': 0.62,
    'OutlineFadeStartFraction': 0.80,
    'MeshGoneFraction': 0.95,
    'PreviewEnabled': False,
    'PreviewTime': 0.0,
}
for name, value in defaults.items():
    cdo.set_editor_property(name, value)

BP.compile_blueprint(bp)
E.save_loaded_asset(bp)


# Create a clean skeletal target sample without touching Gunner's real Blueprint or materials.
sample = E.load_asset(ROOT + '/BP_RadiantSkeletalTargetSample')
if not sample:
    sample = BP.create(ROOT, 'BP_RadiantSkeletalTargetSample', u.Actor.static_class())
    skeletal_component = AT.add_component(sample, u.SkeletalMeshComponent.static_class(), 'SkeletalTarget')
    skeletal_component.set_skeletal_mesh_asset(E.load_asset('/Game/MineLearning/Characters/Gunner/Meshes/SK_Gunner'))
    skeletal_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    skeletal_component.set_editor_property('cast_shadow', False)
    BP.compile_blueprint(sample)
    E.save_loaded_asset(sample)


# Reset the placed controller to adaptive defaults while keeping the user's selected AK test mesh.
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label() != 'RadiantDissolve_Test':
        continue
    for name, value in defaults.items():
        actor.set_editor_property(name, value)
    target = next((component for component in actor.get_components_by_class(u.StaticMeshComponent) if component.get_name().startswith('TargetMesh')), None)
    if target and target.static_mesh:
        for slot_index, slot in enumerate(target.static_mesh.get_editor_property('static_materials')):
            target.set_material(slot_index, slot.get_editor_property('material_interface'))
    actor.call_method('Reset')

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_V2_CORE_READY')

