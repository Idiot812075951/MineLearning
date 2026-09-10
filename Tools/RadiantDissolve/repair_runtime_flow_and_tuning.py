"""Repair the reusable play/spawn chain and apply the reviewed prototype tuning."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
library = u.EditorAssetLibrary
bp = library.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)


def write(name, code):
    BP.write_graph_dsl(BP.get_graph(bp, name), code)


try:
    preview_graph = BP.get_graph(bp, 'RadiantEnablePreview')
except Exception:
    preview_graph = None
if not preview_graph:
    BP.add_function_graph(bp, 'RadiantEnablePreview')
    BP.compile_blueprint(bp)


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

write('RadiantPlay', r'''(fn RadiantPlay ()
 (CallFunction|RadiantReset)
 (CallFunction|RadiantPrepare)
 (CallFunction|RadiantSpawnParticles)
 (变量|RadiantInternal|SetRadiantStartTime (Utilities|Time|GetGameTimeinSeconds))
 (Utilities|Time|SetTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "RadiantUpdate" :Time 0.0166667 :bLooping true :bMaxOncePerFrame true))''')

write('RadiantEnablePreview', r'''(fn RadiantEnablePreview ()
 (Utilities|Time|ClearTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "RadiantUpdate")
 (变量|RadiantV2|SetRadiantPreviewEnabled true)
 (CallFunction|RadiantApplyFrame))''')

write('RadiantReset', r'''(fn RadiantReset ()
 (Utilities|Time|ClearTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "RadiantUpdate")
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Components|DestroyComponent :self fx))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantActiveParticles))
 (CallFunction|RadiantRestore)
 (变量|RadiantInternal|SetRadiantUsingSkeletalFadeMaterial false)
 (CallFunction|RadiantResolve)
 (CallFunction|RadiantPlaceOrigin)
 (CallFunction|RadiantComputeCoverage)
 (变量|RadiantInternal|SetRadiantElapsed 0)
 (变量|RadiantV2|SetRadiantPreviewEnabled false)
 (CallFunction|RadiantApplyFrame))''')

# Rebuild the two direct test shortcuts after graph rewrites.  Key events are
# entry nodes, so their Pressed pins must be connected explicitly.
write('EventGraph', r'''(event EventBeginPlay
 (CallFunction|RadiantReset)
 (Input|EnableInput :PlayerController (Game|GetPlayerController :PlayerIndex 0)))''')
event_graph = BP.get_graph(bp, 'EventGraph')
for key, function_name, y in [('P', 'RadiantPlay', 420), ('R', 'RadiantReset', 700)]:
    event_infos = BP.get_node_infos(BP.find_nodes(event_graph, key, entry_points_only=False))
    for info in event_infos:
        if info.type_id == '输入|KeyboardEvents|' + key:
            BP.delete_node(info.node)
    call_infos = BP.get_node_infos(BP.find_nodes(event_graph, function_name, entry_points_only=False))
    for info in call_infos:
        if info.position.y >= 300 and info.type_id.endswith(function_name):
            BP.delete_node(info.node)
    event_node = BP.create_node(
        event_graph, '输入|KeyboardEvents|' + key, u.IntPoint(-120, y)
    )
    call_node = BP.create_node(event_graph, 'CallFunction|' + function_name, u.IntPoint(320, y))
    event_info, call_info = BP.get_node_infos([event_node, call_node])
    pressed = next(pin.pin_id for pin in event_info.output_pins if pin.name == 'Pressed')
    execute = next(pin.pin_id for pin in call_info.input_pins if pin.type_id == 'Exec')
    BP.connect_pins(pressed, execute)

BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
defaults = {
    'RadiantDuration': 2.4,
    'RadiantDissolveStart': 0.24,
    'RadiantOutlineHoldStart': 0.66,
    'RadiantOutlineFadeStart': 0.84,
    'RadiantMeshGone': 0.96,
    'RadiantParticleRate': 2600.0,
    'RadiantParticleFrontWidth': 4.5,
    'RadiantPreviewEnabled': False,
    'RadiantPreviewTime': 0.0,
}
for name, value in defaults.items():
    cdo.set_editor_property(name, value)
BP.compile_blueprint(bp, warnings_as_errors=True)
library.save_loaded_asset(bp)

# Keep the user's hand-tuned origin height on the placed controller.  Only apply
# phase/particle defaults and restore a clean initial state.
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() != 'BP_RadiantDissolve_Test_C':
        continue
    for name, value in defaults.items():
        actor.set_editor_property(name, value)
    actor.call_method('RadiantReset')
    internal_mesh = next(
        (component for component in actor.get_components_by_class(u.StaticMeshComponent)
         if component.get_name().startswith('TargetMesh')),
        None,
    )
    if internal_mesh and internal_mesh.static_mesh:
        for index, slot in enumerate(internal_mesh.static_mesh.get_editor_property('static_materials')):
            internal_mesh.set_material(index, slot.get_editor_property('material_interface'))

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_RUNTIME_FLOW_AND_TUNING_REPAIRED')
