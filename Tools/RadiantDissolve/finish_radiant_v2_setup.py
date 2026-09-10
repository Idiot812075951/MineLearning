"""Finish setup after graph authoring: origin graph, input links, defaults and saves."""
import unreal as u
from editor_toolset.toolsets.actor import ActorTools as AT
from editor_toolset.toolsets.blueprint import BlueprintTools as BP

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
E = u.EditorAssetLibrary
bp = E.load_asset(ROOT + '/BP_RadiantDissolve_Test')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantPlaceOrigin'), r'''(fn RadiantPlaceOrigin ()
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

# The existing P/R nodes already call the small compatibility wrappers. Remove stale
# prototype events and the unconnected node left by the interrupted authoring pass.
event_graph = BP.get_graph(bp, 'EventGraph')
for info in BP.get_node_infos(BP.find_nodes(event_graph, '')):
    delete_unused_event = info.type_id in {
        'AddEvent|Collision|EventActorBeginOverlap',
        'AddEvent|EventTick',
    }
    delete_unconnected_call = (
        info.type_id == '|RadiantPlay'
        and all(not pin.connected_pins for pin in info.input_pins if pin.type_id == 'Exec')
    )
    if delete_unused_event or delete_unconnected_call:
        BP.delete_node(info.node)

BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
defaults = {
    'RadiantFallbackMaterial': E.load_asset(ROOT + '/M_RadiantDissolve_Surface'),
    'RadiantSkeletalFallbackMaterial': E.load_asset(ROOT + '/M_RadiantDissolve_Gunner'),
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
    'RadiantParticleRate': 7200.0,
    'RadiantParticleFrontWidth': 9.0,
    'RadiantPreviewEnabled': False,
    'RadiantPreviewTime': 0.0,
}
for name, value in defaults.items():
    cdo.set_editor_property(name, value)

marker = next(
    component for component in AT.get_components(cdo)
    if component.get_name().startswith('OriginMarker')
)
marker.set_editor_property('hidden_in_game', False)
marker.set_visibility(True, True)
default_target = next(
    component for component in AT.get_components(cdo)
    if component.get_name().startswith('TargetMesh')
)
default_target.set_relative_scale3d(u.Vector(3.0, 3.0, 3.0))

BP.compile_blueprint(bp, warnings_as_errors=True)
E.save_loaded_asset(bp)

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label() == 'ExternalTargetSample':
        actor.set_actor_location(u.Vector(-320.0, 0.0, 0.0), False, False)
    if actor.get_actor_label() == 'SkeletalDissolve_Sample':
        actor.set_actor_location(u.Vector(320.0, 0.0, 0.0), False, False)
    if actor.get_actor_label() != 'RadiantDissolve_Test':
        continue
    for name, value in defaults.items():
        actor.set_editor_property(name, value)
    actor.set_editor_property('RadiantTarget', None)
    actor.call_method('RadiantReset')
    target = next(
        (component for component in actor.get_components_by_class(u.StaticMeshComponent)
         if component.get_name().startswith('TargetMesh')),
        None,
    )
    if target and target.static_mesh:
        target.set_relative_scale3d(u.Vector(3.0, 3.0, 3.0))
        for slot_index, slot in enumerate(target.static_mesh.get_editor_property('static_materials')):
            target.set_material(slot_index, slot.get_editor_property('material_interface'))

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
for path in [ROOT + '/NS_RadiantDissolve_Test', ROOT + '/NS_RadiantDissolve_Skeletal']:
    asset = E.load_asset(path)
    if asset:
        E.save_loaded_asset(asset)
print('RADIANT_V2_FINAL_READY')
