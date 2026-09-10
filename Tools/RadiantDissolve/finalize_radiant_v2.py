"""Finalize the reusable static/skeletal radiant dissolve Blueprint graphs."""
import unreal as u
from editor_toolset.toolsets.actor import ActorTools as AT
from editor_toolset.toolsets.blueprint import BlueprintTools as BP

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
E = u.EditorAssetLibrary
bp = u.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)


def write(name, code):
    BP.write_graph_dsl(BP.get_graph(bp, name), code)
    print('RADIANT_FINAL_GRAPH', name)


write('RadiantResolve', r'''(fn RadiantResolve ()
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantStaticMeshes))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantSkeletalMeshes))
 (Utilities|IsValid (变量|RadiantV2|GetRadiantTarget)
  (:"Is Valid"
   (Rendering|SetVisibility :self (变量|Default|GetTargetMesh) :bNewVisibility false)
   (for component (Actor|GetComponentsByClass :self (变量|RadiantV2|GetRadiantTarget) :ComponentClass "/Script/Engine.StaticMeshComponent")
    (bind mesh (工具|Casting|CastToStaticMeshComponent :Object component)
     (:then (Utilities|Array|Add (变量|RadiantInternal|GetRadiantStaticMeshes) mesh))
     (:CastFailed)))
   (for component (Actor|GetComponentsByClass :self (变量|RadiantV2|GetRadiantTarget) :ComponentClass "/Script/Engine.SkeletalMeshComponent")
    (bind mesh (工具|Casting|CastToSkeletalMeshComponent :Object component)
     (:then (Utilities|Array|Add (变量|RadiantInternal|GetRadiantSkeletalMeshes) mesh))
     (:CastFailed))))
  (:"Is Not Valid"
   (Rendering|SetVisibility :self (变量|Default|GetTargetMesh) :bNewVisibility true)
   (Utilities|Array|Add (变量|RadiantInternal|GetRadiantStaticMeshes) (变量|Default|GetTargetMesh)))))''')


write('RadiantPlaceOrigin', r'''(fn RadiantPlaceOrigin ()
 (bind current (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
 (Utilities|IsValid (变量|RadiantV2|GetRadiantTarget)
  (:"Is Valid"
   (bind (validCenter validExtent) (Collision|GetActorBounds :self (变量|RadiantV2|GetRadiantTarget) :bOnlyCollidingComponents false :bIncludeFromChildActors true))
   (bind (validX validY validZ) (Math|Vector|BreakVector validExtent))
   (bind validAutomatic (+ validCenter (Math|Vector|MakeVector :X 0 :Y 0 :Z (* validZ (变量|RadiantV2|GetRadiantOriginHeightRatio)))))
   (Transformation|SetWorldLocation :self (变量|Default|GetDissolveOrigin) :NewLocation (工具|Select current validAutomatic (变量|RadiantV2|GetRadiantAutoPlaceOrigin)) :bSweep false :bTeleport true)
   (Rendering|SetVisibility :self (变量|Default|GetOriginMarker) :bNewVisibility (变量|RadiantV2|GetRadiantShowOriginMarker) :bPropagateToChildren true))
  (:"Is Not Valid"
   (bind (fallbackCenter fallbackExtent fallbackRadius) (Collision|GetComponentBounds (变量|Default|GetTargetMesh)))
   (bind (fallbackX fallbackY fallbackZ) (Math|Vector|BreakVector fallbackExtent))
   (bind fallbackAutomatic (+ fallbackCenter (Math|Vector|MakeVector :X 0 :Y 0 :Z (* fallbackZ (变量|RadiantV2|GetRadiantOriginHeightRatio)))))
   (Transformation|SetWorldLocation :self (变量|Default|GetDissolveOrigin) :NewLocation (工具|Select current fallbackAutomatic (变量|RadiantV2|GetRadiantAutoPlaceOrigin)) :bSweep false :bTeleport true)
   (Rendering|SetVisibility :self (变量|Default|GetOriginMarker) :bNewVisibility (变量|RadiantV2|GetRadiantShowOriginMarker) :bPropagateToChildren true))))''')


write('RadiantComputeCoverage', r'''(fn RadiantComputeCoverage ()
 (变量|RadiantInternal|SetRadiantEffectiveRadius 1)
 (bind origin (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
 (for mesh (变量|RadiantInternal|GetRadiantStaticMeshes)
  (bind (center extent radius) (Collision|GetComponentBounds mesh))
  (bind reach (+ (+ (Math|Vector|Distance(Vector) center origin) (Math|Vector|VectorLength extent)) 2))
  (变量|RadiantInternal|SetRadiantEffectiveRadius (工具|Select (变量|RadiantInternal|GetRadiantEffectiveRadius) reach (> reach (变量|RadiantInternal|GetRadiantEffectiveRadius)))))
 (for mesh (变量|RadiantInternal|GetRadiantSkeletalMeshes)
  (bind (center extent radius) (Collision|GetComponentBounds mesh))
  (bind reach (+ (+ (Math|Vector|Distance(Vector) center origin) (Math|Vector|VectorLength extent)) 2))
  (变量|RadiantInternal|SetRadiantEffectiveRadius (工具|Select (变量|RadiantInternal|GetRadiantEffectiveRadius) reach (> reach (变量|RadiantInternal|GetRadiantEffectiveRadius))))))''')


write('RadiantPrepareStatic', r'''(fn RadiantPrepareStatic ()
 (for mesh (变量|RadiantInternal|GetRadiantStaticMeshes)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantStaticMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (Utilities|Array|Add (变量|RadiantInternal|GetRadiantOriginalStaticMaterials) (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|RadiantV2|GetRadiantFallbackMaterial)))))''')


write('RadiantPrepareSkeletal', r'''(fn RadiantPrepareSkeletal ()
 (for mesh (变量|RadiantInternal|GetRadiantSkeletalMeshes)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantSkeletalMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (Utilities|Array|Add (变量|RadiantInternal|GetRadiantOriginalSkeletalMaterials) (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|RadiantV2|GetRadiantSkeletalFallbackMaterial)))))''')


write('RadiantPrepare', r'''(fn RadiantPrepare ()
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantOriginalStaticMaterials))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantOriginalSkeletalMaterials))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantStaticMaterialCounts))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantSkeletalMaterialCounts))
 (CallFunction|RadiantPrepareStatic)
 (CallFunction|RadiantPrepareSkeletal))''')


write('RadiantRestoreStatic', r'''(fn RadiantRestoreStatic ()
 (变量|RadiantInternal|SetRadiantMaterialCursor 0)
 (for meshIndex (range (Utilities|Array|Length (变量|RadiantInternal|GetRadiantStaticMaterialCounts)))
  (bind mesh (Utilities|Array|Get(acopy) (变量|RadiantInternal|GetRadiantStaticMeshes) meshIndex))
  (for slot (range (Utilities|Array|Get(acopy) (变量|RadiantInternal|GetRadiantStaticMaterialCounts) meshIndex))
   (Rendering|Material|SetMaterial :self mesh :ElementIndex slot :Material (Utilities|Array|Get(acopy) (变量|RadiantInternal|GetRadiantOriginalStaticMaterials) (变量|RadiantInternal|GetRadiantMaterialCursor)))
   (变量|RadiantInternal|SetRadiantMaterialCursor (+ (变量|RadiantInternal|GetRadiantMaterialCursor) 1)))))''')


write('RadiantRestoreSkeletal', r'''(fn RadiantRestoreSkeletal ()
 (变量|RadiantInternal|SetRadiantMaterialCursor 0)
 (for meshIndex (range (Utilities|Array|Length (变量|RadiantInternal|GetRadiantSkeletalMaterialCounts)))
  (bind mesh (Utilities|Array|Get(acopy) (变量|RadiantInternal|GetRadiantSkeletalMeshes) meshIndex))
  (for slot (range (Utilities|Array|Get(acopy) (变量|RadiantInternal|GetRadiantSkeletalMaterialCounts) meshIndex))
   (Rendering|Material|SetMaterial :self mesh :ElementIndex slot :Material (Utilities|Array|Get(acopy) (变量|RadiantInternal|GetRadiantOriginalSkeletalMaterials) (变量|RadiantInternal|GetRadiantMaterialCursor)))
   (变量|RadiantInternal|SetRadiantMaterialCursor (+ (变量|RadiantInternal|GetRadiantMaterialCursor) 1)))))''')


write('RadiantRestore', r'''(fn RadiantRestore ()
 (CallFunction|RadiantRestoreStatic)
 (CallFunction|RadiantRestoreSkeletal)
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantOriginalStaticMaterials))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantOriginalSkeletalMaterials))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantStaticMaterialCounts))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantSkeletalMaterialCounts)))''')


write('RadiantSpawnStaticParticles', r'''(fn RadiantSpawnStaticParticles ()
 (for mesh (变量|RadiantInternal|GetRadiantStaticMeshes)
  (bind fx (Niagara|SpawnSystemAttached :SystemTemplate "/Game/MineLearning/VFX/RadiantDissolve/NS_RadiantDissolve_Test.NS_RadiantDissolve_Test" :AttachToComponent mesh :bAutoDestroy false :bAutoActivate false :bPreCullCheck false))
  (Niagara|SetNiagaraStaticMeshComponent :NiagaraSystem fx :OverrideName "User.TargetMesh" :StaticMeshComponent mesh)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantActiveParticles) fx)))''')


write('RadiantSpawnSkeletalParticles', r'''(fn RadiantSpawnSkeletalParticles ()
 (for mesh (变量|RadiantInternal|GetRadiantSkeletalMeshes)
  (bind fx (Niagara|SpawnSystemAttached :SystemTemplate "/Game/MineLearning/VFX/RadiantDissolve/NS_RadiantDissolve_Skeletal.NS_RadiantDissolve_Skeletal" :AttachToComponent mesh :bAutoDestroy false :bAutoActivate false :bPreCullCheck false))
  (Niagara|SetNiagaraSkeletalMeshComponent :NiagaraSystem fx :OverrideName "User.TargetSkeletalMesh" :SkeletalMeshComponent mesh)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantActiveParticles) fx)))''')


write('RadiantSpawnParticles', r'''(fn RadiantSpawnParticles ()
 (CallFunction|RadiantSpawnStaticParticles)
 (CallFunction|RadiantSpawnSkeletalParticles)
 (CallFunction|RadiantApplyFrame)
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Components|Activation|Activate :self fx :bReset true)))''')


apply_static = r'''
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
apply_skeletal = apply_static.replace('GetRadiantStaticMeshes', 'GetRadiantSkeletalMeshes')

write('RadiantApplyFrame', r'''(fn RadiantApplyFrame ()
 (bind duration (变量|RadiantV2|GetRadiantDuration))
 (bind safeDuration (工具|Select 0.1 duration (> duration 0.1)))
 (bind rawT (/ (变量|RadiantInternal|GetRadiantElapsed) safeDuration))
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
 (bind perMeshRate (/ (* (变量|RadiantV2|GetRadiantParticleRate) (+ 0.35 (* particleRamp 1.65))) (工具|Select 1 componentCount (> componentCount 0))))
 (bind frontWidth (* (变量|RadiantV2|GetRadiantParticleFrontWidth) (+ 1 (* late 1.2))))
 (bind origin (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
''' + apply_static + apply_skeletal + r'''
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Niagara|SetNiagaraVariable(Position) :self fx :InVariableName "User.DissolveOriginWS" :InValue origin)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.DissolveRadius" :InValue dissolve)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.NoiseStrength" :InValue noise)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.FrontWidth" :InValue frontWidth)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.SpawnRate" :InValue perMeshRate)))''')


write('RadiantReset', r'''(fn RadiantReset ()
 (Utilities|Time|ClearTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "RadiantUpdate")
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Components|DestroyComponent :self fx))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantActiveParticles))
 (CallFunction|RadiantRestore)
 (CallFunction|RadiantResolve)
 (CallFunction|RadiantPlaceOrigin)
 (CallFunction|RadiantComputeCoverage)
 (变量|RadiantInternal|SetRadiantElapsed 0)
 (变量|RadiantV2|SetRadiantPreviewTime 0)
 (变量|RadiantV2|SetRadiantPreviewEnabled false)
 (CallFunction|RadiantApplyFrame))''')


write('RadiantPlay', r'''(fn RadiantPlay ()
 (CallFunction|RadiantReset)
 (CallFunction|RadiantPrepare)
 (CallFunction|RadiantSpawnParticles)
 (变量|RadiantInternal|SetRadiantStartTime (Utilities|Time|GetGameTimeinSeconds))
 (Utilities|Time|SetTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "RadiantUpdate" :Time 0.0166667 :bLooping true :bMaxOncePerFrame true))''')


write('RadiantFinish', r'''(fn RadiantFinish ()
 (Utilities|Time|ClearTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "RadiantUpdate")
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Niagara|SetNiagaraVariable(Float) :self fx :InVariableName "User.SpawnRate" :InValue 0)
  (Components|Activation|Deactivate :self fx)))''')


write('RadiantNoOp', r'''(fn RadiantNoOp ())''')


write('RadiantUpdate', r'''(fn RadiantUpdate ()
 (变量|RadiantInternal|SetRadiantElapsed (- (Utilities|Time|GetGameTimeinSeconds) (变量|RadiantInternal|GetRadiantStartTime)))
 (CallFunction|RadiantApplyFrame)
 (工具|流程控制|Branch (>= (变量|RadiantInternal|GetRadiantElapsed) (变量|RadiantV2|GetRadiantDuration))
  (:then (CallFunction|RadiantFinish))
  (:else (CallFunction|RadiantNoOp))))''')


# Keep the original public entry names working for the existing demo GameMode.
write('TestDissolve', r'''(fn TestDissolve ()
 (CallFunction|RadiantPlay))''')
write('Reset', r'''(fn Reset ()
 (CallFunction|RadiantReset))''')
write('UpdateDissolve', r'''(fn UpdateDissolve ()
 (CallFunction|RadiantUpdate))''')
write('FinishDissolve', r'''(fn FinishDissolve ()
 (CallFunction|RadiantFinish))''')


write('UserConstructionScript', r'''(fn ConstructionScript ()
 (CallFunction|RadiantResolve)
 (CallFunction|RadiantPlaceOrigin)
 (CallFunction|RadiantComputeCoverage)
 (变量|RadiantInternal|SetRadiantElapsed 0))''')


# BeginPlay plus direct P/R input events. Key events are created explicitly because
# they have no input execution pin and are clearer as separate two-node chains.
write('EventGraph', r'''(event EventBeginPlay
 (CallFunction|RadiantReset)
 (Input|EnableInput :PlayerController (Game|GetPlayerController :PlayerIndex 0)))''')
event_graph = BP.get_graph(bp, 'EventGraph')
for key, function_name, y in [('P', 'RadiantPlay', 420), ('R', 'RadiantReset', 700)]:
    event_infos = BP.get_node_infos(BP.find_nodes(event_graph, key, entry_points_only=True))
    event_node = next(info.node for info in event_infos if info.type_id == '输入|KeyboardEvents|' + key)
    call_node = BP.create_node(event_graph, 'CallFunction|' + function_name, u.IntPoint(320, y))
    event_info, call_info = BP.get_node_infos([event_node, call_node])
    pressed = next(pin.pin_id for pin in event_info.output_pins if pin.name == 'Pressed')
    execute = next(pin.pin_id for pin in call_info.input_pins if pin.type_id == 'Exec')
    BP.connect_pins(pressed, execute)


BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
defaults = {
    'RadiantFallbackMaterial': E.load_asset(ROOT + '/M_RadiantDissolve_Surface'),
    'RadiantAutoPlaceOrigin': True,
    'RadiantShowOriginMarker': True,
    'RadiantOriginHeightRatio': 0.22,
    'RadiantDuration': 2.4,
    'RadiantTravelDistance': 0.0,
    'RadiantNoiseStrength': 12.0,
    'RadiantHeatIntensity': 88.0,
    'RadiantEdgeWidth': 4.0,
    'RadiantDissolveStart': 0.24,
    'RadiantOutlineHoldStart': 0.66,
    'RadiantOutlineFadeStart': 0.80,
    'RadiantMeshGone': 0.95,
    'RadiantParticleRate': 4200.0,
    'RadiantParticleFrontWidth': 8.5,
    'RadiantPreviewEnabled': False,
    'RadiantPreviewTime': 0.0,
}
for name, value in defaults.items():
    cdo.set_editor_property(name, value)

components = AT.get_components(cdo)
marker = next(component for component in components if component.get_name().startswith('OriginMarker'))
marker.set_editor_property('hidden_in_game', False)
marker.set_visibility(True, True)

BP.compile_blueprint(bp, warnings_as_errors=True)
E.save_loaded_asset(bp)


# Reset the placed test controller and restore the AK's own material slots in the map.
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label() != 'RadiantDissolve_Test':
        continue
    for name, value in defaults.items():
        actor.set_editor_property(name, value)
    actor.set_editor_property('radiant_target', None)
    target = next(
        (component for component in actor.get_components_by_class(u.StaticMeshComponent)
         if component.get_name().startswith('TargetMesh')),
        None,
    )
    if target and target.static_mesh:
        for slot_index, slot in enumerate(target.static_mesh.get_editor_property('static_materials')):
            target.set_material(slot_index, slot.get_editor_property('material_interface'))
    actor.call_method('RadiantReset')

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
for path in [ROOT + '/NS_RadiantDissolve_Test', ROOT + '/NS_RadiantDissolve_Skeletal']:
    asset = E.load_asset(path)
    if asset:
        E.save_loaded_asset(asset)
print('RADIANT_V2_FINAL_READY')
