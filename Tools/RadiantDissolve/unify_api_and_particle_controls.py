"""Simplify the public dissolve API and delay particle emission.

The caller gets one target/origin entry point. Mesh type, material slots and
Niagara data interfaces stay internal to the prototype actor.
"""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
BLUEPRINT_PATH = ROOT + '/BP_RadiantDissolve_Test'
LIBRARY = u.EditorAssetLibrary


bp = LIBRARY.load_asset(BLUEPRINT_PATH)
if not bp:
    raise RuntimeError('BP_RadiantDissolve_Test is missing')

level_editor = u.get_editor_subsystem(u.LevelEditorSubsystem)

u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)


# Remove the abandoned V1 surface. Nothing in the current map calls these
# graphs; keeping them made the actor look like two competing implementations.
legacy_functions = [
    'Initialize', 'TestDissolve', 'Reset', 'UpdateDissolve', 'ApplyFrame',
    'ResolveTargets', 'PrepareMaterials', 'RestoreMaterials', 'SpawnParticles',
    'FinishDissolve', 'PreviewFrame', 'UpdateAutoOrigin',
    'RadiantPrepareSkeletalFade', 'RadiantUpdateSkeletalFadeStage',
]
function_names = {entry.name for entry in BP.list_functions(bp)}
for name in legacy_functions:
    if name in function_names:
        BP.remove_function_graph(bp, name)

legacy_variables = [
    'Elapsed', 'Duration', 'PropagationDistance', 'NoiseStrength',
    'HeatIntensity', 'DissolveEdgeWidth', 'PreviewTime', 'DynamicMaterial',
    'StartTime', 'TargetActor', 'BoundMeshes', 'OriginalMaterials',
    'MaterialCounts', 'ActiveParticles', 'MaterialCursor', 'EffectiveRadius',
    'ParticleSampleRate', 'PreviewEnabled', 'BoundSkeletalMeshes',
    'OriginalSkeletalMaterials', 'FallbackMaterial', 'AutoPlaceOrigin',
    'OriginHeightRatio', 'ParticleFrontWidth', 'DissolveStartFraction',
    'OutlineHoldStartFraction', 'OutlineFadeStartFraction',
    'MeshGoneFraction', 'SkeletalMaterialCounts', 'OriginReference',
    'RadiantPhase', 'RadiantSkeletalFadeMaterial',
    'RadiantUsingSkeletalFadeMaterial',
]
variable_names = set(BP.list_variables(bp))
for name in legacy_variables:
    if name in variable_names:
        BP.remove_variable(bp, name)


for name in ['RadiantParticleStart', 'RadiantParticleSpeed']:
    if name not in BP.list_variables(bp):
        BP.add_variable(bp, name, 'float')


# Only these settings describe the requested result. Everything else is an
# implementation detail or a fixed part of this one VFX design.
public_variables = {
    'RadiantShowOriginMarker',
    'RadiantDuration',
    'RadiantNoiseStrength',
    'RadiantHeatIntensity',
    'RadiantEdgeWidth',
    'RadiantParticleRate',
    'RadiantParticleStart',
    'RadiantParticleSpeed',
}
for name in BP.list_variables(bp):
    BP.set_variable_instance_editable(bp, name, name in public_variables)
    BP.set_variable_category(
        bp,
        name,
        'Radiant Dissolve' if name in public_variables else 'Radiant Internal',
    )


# Public call: one Actor and one world-space contact point. Static/Skeletal and
# material-slot handling remains behind RadiantPlay.
if 'Dissolve' not in {entry.name for entry in BP.list_functions(bp)}:
    graph = BP.add_function_graph(bp, 'Dissolve')
    BP.add_object_function_param(graph, 'Target', u.Actor.static_class(), True)
    BP.add_struct_function_param(graph, 'OriginWS', u.Vector.static_struct(), True)
else:
    graph = BP.get_graph(bp, 'Dissolve')

BP.write_graph_dsl(graph, r'''(fn Dissolve (Target OriginWS)
 (变量|RadiantInternal|SetRadiantTarget Target)
 (变量|RadiantInternal|SetRadiantAutoPlaceOrigin false)
 (Transformation|SetWorldLocation :self (变量|Default|GetDissolveOrigin) :NewLocation OriginWS)
 (CallFunction|RadiantPlay))''')


# No particles during the red-outline/erosion phase. ParticleStart opens one
# emission window near the final outline fade, at full requested rate.
BP.write_graph_dsl(BP.get_graph(bp, 'RadiantApplyFrame'), r'''(fn RadiantApplyFrame ()
 (bind duration (变量|RadiantDissolve|GetRadiantDuration))
 (bind safeDuration (工具|Select 0.1 duration (> duration 0.1)))
 (bind elapsedT (/ (变量|RadiantInternal|GetRadiantElapsed) safeDuration))
 (bind rawT (工具|Select elapsedT (变量|RadiantInternal|GetRadiantPreviewTime) (变量|RadiantInternal|GetRadiantPreviewEnabled)))
 (bind t (工具|Select (工具|Select rawT 1 (> rawT 1)) 0 (< rawT 0)))
 (bind automaticReach (变量|RadiantInternal|GetRadiantEffectiveRadius))
 (bind reach (工具|Select automaticReach (变量|RadiantInternal|GetRadiantTravelDistance) (> (变量|RadiantInternal|GetRadiantTravelDistance) 0)))
 (bind rawNoise (变量|RadiantDissolve|GetRadiantNoiseStrength))
 (bind absNoise (工具|Select (- 0 rawNoise) rawNoise (> rawNoise 0)))
 (bind maxNoise (* reach 0.22))
 (bind noise (工具|Select absNoise maxNoise (> absNoise maxNoise)))
 (bind edge (变量|RadiantDissolve|GetRadiantEdgeWidth))
 (bind travel (+ reach noise))
 (bind start (变量|RadiantInternal|GetRadiantDissolveStart))
 (bind hold (变量|RadiantInternal|GetRadiantOutlineHoldStart))
 (bind fade (变量|RadiantInternal|GetRadiantOutlineFadeStart))
 (bind gone (变量|RadiantInternal|GetRadiantMeshGone))
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
 (bind componentCount (+ (Utilities|Array|Length (变量|RadiantInternal|GetRadiantStaticMeshes)) (Utilities|Array|Length (变量|RadiantInternal|GetRadiantSkeletalMeshes))))
 (bind particleEnabled (工具|Select 0 1 (>= t (变量|RadiantDissolve|GetRadiantParticleStart))))
 (bind perMeshRate (/ (* (变量|RadiantDissolve|GetRadiantParticleRate) particleEnabled) (工具|Select 1 componentCount (> componentCount 0))))
 (bind frontWidth (* (变量|RadiantInternal|GetRadiantParticleFrontWidth) (+ 1 (* late 0.45))))
 (bind origin (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
 (for mesh (变量|RadiantInternal|GetRadiantStaticMeshes)
  (Rendering|Material|SetVectorParameterValueOnMaterials :self mesh :ParameterName "DissolveOriginWS" :ParameterValue origin)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveProgress" :ParameterValue t)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatRadius" :ParameterValue heat)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveRadius" :ParameterValue dissolve)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "NoiseStrength" :ParameterValue noise)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatIntensity" :ParameterValue (变量|RadiantDissolve|GetRadiantHeatIntensity))
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveEdgeWidth" :ParameterValue edge)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OutlineAmount" :ParameterValue outline)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OpacityFade" :ParameterValue opacity))
 (for mesh (变量|RadiantInternal|GetRadiantSkeletalMeshes)
  (Rendering|Material|SetVectorParameterValueOnMaterials :self mesh :ParameterName "DissolveOriginWS" :ParameterValue origin)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveProgress" :ParameterValue t)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatRadius" :ParameterValue heat)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveRadius" :ParameterValue dissolve)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "NoiseStrength" :ParameterValue noise)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "HeatIntensity" :ParameterValue (变量|RadiantDissolve|GetRadiantHeatIntensity))
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "DissolveEdgeWidth" :ParameterValue edge)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OutlineAmount" :ParameterValue outline)
  (Rendering|Material|SetScalarParameterValueOnMaterials :self mesh :ParameterName "OpacityFade" :ParameterValue opacity))
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Niagara|SetNiagaraVariable(Position) :self fx :InVariableName "User.DissolveOriginWS" :InValue origin)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.DissolveRadius" :InValue dissolve)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.NoiseStrength" :InValue noise)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.FrontWidth" :InValue frontWidth)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.ParticleSpeed" :InValue (变量|RadiantDissolve|GetRadiantParticleSpeed))
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.SpawnRate" :InValue perMeshRate)))''')

# Remove the old raw P/R shortcuts. They made console typing mutate the effect
# and created a second way to start it beside the one-call public API.
event_graph = BP.get_graph(bp, 'EventGraph')
for key in ['P', 'R']:
    for info in BP.get_node_infos(BP.find_nodes(event_graph, key, entry_points_only=False)):
        if info.type_id == '输入|KeyboardEvents|' + key:
            BP.delete_node(info.node)
BP.write_graph_dsl(event_graph, r'''(event EventBeginPlay
 (CallFunction|RadiantReset))''')


BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
cdo.set_editor_property('RadiantParticleStart', 0.84)
cdo.set_editor_property('RadiantParticleSpeed', 36.0)
cdo.set_editor_property('RadiantParticleRate', 7500.0)
BP.compile_blueprint(bp, warnings_as_errors=True)
LIBRARY.save_loaded_asset(bp)


# Preserve the user's placed origin while applying the new look defaults.
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() != 'BP_RadiantDissolve_Test_C':
        continue
    actor.set_editor_property('RadiantParticleStart', 0.84)
    actor.set_editor_property('RadiantParticleSpeed', 36.0)
    actor.set_editor_property('RadiantParticleRate', 7500.0)
    actor.call_method('RadiantReset')

level_editor.save_current_level()
print('RADIANT_UNIFIED_API_AND_PARTICLE_CONTROLS_APPLIED')
