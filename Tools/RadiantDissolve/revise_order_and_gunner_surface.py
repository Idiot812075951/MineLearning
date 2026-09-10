"""Put the red silhouette before erosion and preserve the Gunner source look."""
import unreal as u
from editor_toolset.toolsets.actor import ActorTools as AT
from editor_toolset.toolsets.blueprint import BlueprintTools as BP

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
        material, u.MaterialExpressionScalarParameter, x, y,
        parameter_name=name, default_value=default, group=group,
    )


def connect(source, destination, input_name, output_name=''):
    if not M.connect_material_expressions(source, output_name, destination, input_name):
        raise RuntimeError(f'Could not connect {source.get_name()} -> {destination.get_name()}.{input_name}')


source_path = '/Game/MineLearning/Characters/Gunner/Materials/M_Gunner'
adapter_path = ROOT + '/M_RadiantDissolve_GunnerSurface'
adapter = E.load_asset(adapter_path)
if not adapter:
    adapter = E.duplicate_asset(source_path, adapter_path)
if not adapter:
    raise RuntimeError('Could not create the Gunner dissolve material adapter')

already_built = any(
    isinstance(node, u.MaterialExpressionCustom)
    and node.get_editor_property('description') == 'Preserve Gunner surface while heating'
    for node in M.get_material_expressions(adapter)
)
if not already_built:
    base_node = M.get_material_property_input_node(adapter, u.MaterialProperty.MP_BASE_COLOR)
    base_output = M.get_material_property_input_node_output_name(adapter, u.MaterialProperty.MP_BASE_COLOR)
    if not base_node:
        raise RuntimeError('M_Gunner has no Base Color input to preserve')

    adapter.set_editor_property('blend_mode', u.BlendMode.BLEND_MASKED)
    adapter.set_editor_property('used_with_skeletal_mesh', True)
    function_call = expression(
        adapter, u.MaterialExpressionMaterialFunctionCall, -520, 520,
        material_function=E.load_asset(ROOT + '/MF_RadiantDissolve'),
    )
    outline = scalar(adapter, 'OutlineAmount', 0.0, -520, 720)
    opacity_fade = scalar(adapter, 'OpacityFade', 1.0, -520, 850)
    intensity = scalar(adapter, 'HeatIntensity', 70.0, -520, 980)
    fresnel = expression(adapter, u.MaterialExpressionFresnel, -280, 760)

    surface = expression(
        adapter, u.MaterialExpressionCustom, 80, 300,
        description='Preserve Gunner surface while heating',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
    )
    surface.set_editor_property('inputs', [custom_input(name) for name in ['Base', 'Masks', 'Outline', 'Fresnel']])
    surface.set_editor_property(
        'code',
        'float3 hot=lerp(Base,Base*.24+float3(.31,.002,.008),Masks.x*.9); '
        'float shell=saturate(Outline)*smoothstep(.70,.96,saturate(Fresnel)); '
        'return lerp(hot,float3(.48,.002,.016),shell*.70);',
    )
    connect(base_node, surface, 'Base', base_output)
    connect(function_call, surface, 'Masks')
    connect(outline, surface, 'Outline')
    connect(fresnel, surface, 'Fresnel')

    emission = expression(
        adapter, u.MaterialExpressionCustom, 80, 520,
        description='Gunner heat edge and early red silhouette',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
    )
    emission.set_editor_property('inputs', [custom_input(name) for name in ['Masks', 'Intensity', 'Outline', 'Fresnel']])
    emission.set_editor_property(
        'code',
        'float front=pow(saturate(Masks.y),1.45); '
        'float shell=saturate(Outline)*smoothstep(.68,.94,saturate(Fresnel)); '
        'return Intensity*(pow(saturate(Masks.x),1.45)*float3(1,.008,.018)*.090 '
        '+front*float3(1,.42,.58)*.34+shell*float3(1,.018,.055)*.10);',
    )
    connect(function_call, emission, 'Masks')
    connect(intensity, emission, 'Intensity')
    connect(outline, emission, 'Outline')
    connect(fresnel, emission, 'Fresnel')

    remaining = expression(adapter, u.MaterialExpressionComponentMask, -260, 1080, r=False, g=False, b=True, a=False)
    connect(function_call, remaining, '')
    blue_noise_class = u.load_class(None, '/Script/Engine.MaterialExpressionScalarBlueNoise')
    blue_noise = expression(adapter, blue_noise_class, -260, 1200)
    fade_dither = expression(
        adapter, u.MaterialExpressionCustom, 0, 1160,
        description='Blue-noise Gunner silhouette fade',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
    )
    fade_dither.set_editor_property('inputs', [custom_input('Fade'), custom_input('Noise')])
    fade_dither.set_editor_property('code', 'return step(Noise,Fade);')
    connect(opacity_fade, fade_dither, 'Fade')
    connect(blue_noise, fade_dither, 'Noise')

    held_mask = expression(
        adapter, u.MaterialExpressionCustom, 0, 1020,
        description='Keep Gunner outline after interior erosion',
        output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,
    )
    held_mask.set_editor_property('inputs', [custom_input(name) for name in ['Remaining', 'Outline', 'Fresnel']])
    held_mask.set_editor_property(
        'code', 'float shell=step(.78,saturate(Fresnel))*saturate(Outline); return max(Remaining,shell);',
    )
    connect(remaining, held_mask, 'Remaining')
    connect(outline, held_mask, 'Outline')
    connect(fresnel, held_mask, 'Fresnel')
    faded = expression(adapter, u.MaterialExpressionMultiply, 250, 1080)
    connect(held_mask, faded, 'A')
    connect(fade_dither, faded, 'B')

    M.connect_material_property(surface, '', u.MaterialProperty.MP_BASE_COLOR)
    M.connect_material_property(emission, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    M.connect_material_property(faded, '', u.MaterialProperty.MP_OPACITY_MASK)
    M.recompile_material(adapter)
    E.save_loaded_asset(adapter)

bp = E.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)
if 'RadiantSkeletalFallbackMaterial' not in BP.list_variables(bp):
    BP.add_object_variable(bp, 'RadiantSkeletalFallbackMaterial', u.MaterialInterface.static_class())
BP.set_variable_instance_editable(bp, 'RadiantSkeletalFallbackMaterial', True)
BP.set_variable_category(bp, 'RadiantSkeletalFallbackMaterial', 'RadiantV2')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantPrepareSkeletal'), r'''(fn RadiantPrepareSkeletal ()
 (for mesh (变量|RadiantInternal|GetRadiantSkeletalMeshes)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantSkeletalMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (bind source (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (Utilities|Array|Add (变量|RadiantInternal|GetRadiantOriginalSkeletalMaterials) source)
   (bind mid (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|RadiantV2|GetRadiantSkeletalFallbackMaterial)))
   (Rendering|Material|CopyMaterialInstanceParameters :self mid :Source source :bQuickParametersOnly true))))''')

apply_mesh = r'''
 (for mesh (变量|RadiantInternal|GetRadiantStaticMeshes)
  (Rendering|Material|SetVectorParameterValueOnMaterials :self mesh :ParameterName "DissolveOriginWS" :ParameterValue origin)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveProgress" :ParameterValue t)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatRadius" :ParameterValue heat)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveRadius" :ParameterValue dissolve)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "NoiseStrength" :ParameterValue noise)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatIntensity" :ParameterValue (变量|RadiantV2|GetRadiantHeatIntensity))
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveEdgeWidth" :ParameterValue (变量|RadiantV2|GetRadiantEdgeWidth))
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OutlineAmount" :ParameterValue outline)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OpacityFade" :ParameterValue opacity))'''
apply_skeletal = apply_mesh.replace('GetRadiantStaticMeshes', 'GetRadiantSkeletalMeshes')
BP.write_graph_dsl(BP.get_graph(bp, 'RadiantApplyFrame'), r'''(fn RadiantApplyFrame ()
 (bind duration (变量|RadiantV2|GetRadiantDuration))
 (bind safeDuration (工具|Select 0.1 duration (> duration 0.1)))
 (bind elapsedT (/ (变量|RadiantInternal|GetRadiantElapsed) safeDuration))
 (bind rawT (工具|Select elapsedT (变量|RadiantV2|GetRadiantPreviewTime) (变量|RadiantV2|GetRadiantPreviewEnabled)))
 (bind t (工具|Select (工具|Select rawT 1 (> rawT 1)) 0 (< rawT 0)))
 (bind automaticReach (变量|RadiantInternal|GetRadiantEffectiveRadius))
 (bind reach (工具|Select automaticReach (变量|RadiantV2|GetRadiantTravelDistance) (> (变量|RadiantV2|GetRadiantTravelDistance) 0)))
 (bind rawNoise (变量|RadiantV2|GetRadiantNoiseStrength))
 (bind absNoise (工具|Select (- 0 rawNoise) rawNoise (> rawNoise 0)))
 (bind maxNoise (* reach 0.22))
 (bind noise (工具|Select absNoise maxNoise (> absNoise maxNoise)))
 (bind edge (变量|RadiantV2|GetRadiantEdgeWidth))
 (bind travel (+ reach noise))
 (bind start (变量|RadiantV2|GetRadiantDissolveStart))
 (bind hold (变量|RadiantV2|GetRadiantOutlineHoldStart))
 (bind fade (变量|RadiantV2|GetRadiantOutlineFadeStart))
 (bind gone (变量|RadiantV2|GetRadiantMeshGone))
 (bind earlyRaw (/ (- t start) (工具|Select 0.01 (- hold start) (> (- hold start) 0.01))))
 (bind early (工具|Select (工具|Select earlyRaw 1 (> earlyRaw 1)) 0 (< earlyRaw 0)))
 (bind lateRaw (/ (- t fade) (工具|Select 0.01 (- gone fade) (> (- gone fade) 0.01))))
 (bind late (工具|Select (工具|Select lateRaw 1 (> lateRaw 1)) 0 (< lateRaw 0)))
 (bind front (工具|Select (+ 0.92 (* late 0.08)) (* early 0.92) (< t fade)))
 (bind heatRaw (/ t (工具|Select 0.01 hold (> hold 0.01))))
 (bind heatT (工具|Select (工具|Select heatRaw 1 (> heatRaw 1)) 0 (< heatRaw 0)))
 (bind dissolvePad (+ (+ noise edge) 24))
 (bind heatPad (+ noise 14))
 (bind heat (- (* heatT (+ (+ travel heatPad) 45)) heatPad))
 (bind dissolve (- (* front (+ travel dissolvePad)) dissolvePad))
 (bind opacity (工具|Select (- 1 late) 1 (< t fade)))
 (bind outlineRaw (/ t (工具|Select 0.01 start (> start 0.01))))
 (bind outlineIn (工具|Select (工具|Select outlineRaw 1 (> outlineRaw 1)) 0 (< outlineRaw 0)))
 (bind outline (* outlineIn opacity))
 (bind particleRaw (/ (- t 0.30) (工具|Select 0.01 (- fade 0.30) (> (- fade 0.30) 0.01))))
 (bind particleRamp (工具|Select (工具|Select particleRaw 1 (> particleRaw 1)) 0 (< particleRaw 0)))
 (bind componentCount (+ (Utilities|Array|Length (变量|RadiantInternal|GetRadiantStaticMeshes)) (Utilities|Array|Length (变量|RadiantInternal|GetRadiantSkeletalMeshes))))
 (bind perMeshRate (/ (* (变量|RadiantV2|GetRadiantParticleRate) (+ 0.25 (* particleRamp 1.05))) (工具|Select 1 componentCount (> componentCount 0))))
 (bind frontWidth (* (变量|RadiantV2|GetRadiantParticleFrontWidth) (+ 1 (* late 0.45))))
 (bind origin (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
''' + apply_mesh + apply_skeletal + r'''
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Niagara|SetNiagaraVariable(Position) :self fx :InVariableName "User.DissolveOriginWS" :InValue origin)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.DissolveRadius" :InValue dissolve)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.NoiseStrength" :InValue noise)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.FrontWidth" :InValue frontWidth)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.SpawnRate" :InValue perMeshRate))
 (CallFunction|RadiantUpdateSkeletalFadeStage))''')

BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
cdo.set_editor_property('RadiantSkeletalFallbackMaterial', adapter)
cdo.set_editor_property('RadiantDissolveStart', 0.24)
cdo.set_editor_property('RadiantOutlineHoldStart', 0.66)
BP.compile_blueprint(bp, warnings_as_errors=True)
E.save_loaded_asset(bp)

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label() != 'RadiantDissolve_Test':
        continue
    actor.set_editor_property('RadiantSkeletalFallbackMaterial', adapter)
    actor.set_editor_property('RadiantDissolveStart', 0.24)
    actor.set_editor_property('RadiantOutlineHoldStart', 0.66)
    actor.call_method('RadiantReset')

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_ORDER_AND_GUNNER_SURFACE_REVISED')
