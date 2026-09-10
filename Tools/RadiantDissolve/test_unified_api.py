"""Exercise the one-call public API in PIE."""
import json
from pathlib import Path

import unreal as u


world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(
    None,
    '/Game/MineLearning/VFX/RadiantDissolve/'
    'BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C',
)
controller = u.GameplayStatics.get_all_actors_of_class(world, controller_class)[0]
target = controller.get_editor_property('RadiantTarget')
origin = controller.get_editor_property('DissolveOrigin').get_world_location()
controller.call_method('Dissolve', args=(target, origin))

payload = {
    'target': controller.get_editor_property('RadiantTarget').get_path_name(),
    'static_meshes': len(controller.get_editor_property('RadiantStaticMeshes')),
    'skeletal_meshes': len(controller.get_editor_property('RadiantSkeletalMeshes')),
    'particle_components': len(controller.get_editor_property('RadiantActiveParticles')),
}

static_target = next(
    (
        actor for actor in u.GameplayStatics.get_all_actors_of_class(world, u.Actor)
        if actor.get_actor_label() == 'ExternalTargetSample'
    ),
    None,
)
if static_target:
    static_origin = static_target.get_actor_location() + u.Vector(0.0, 0.0, 30.0)
    controller.call_method('Dissolve', args=(static_target, static_origin))
    payload['second_target'] = static_target.get_path_name()
    payload['second_static_meshes'] = len(
        controller.get_editor_property('RadiantStaticMeshes')
    )
    payload['second_skeletal_meshes'] = len(
        controller.get_editor_property('RadiantSkeletalMeshes')
    )
    payload['second_particle_components'] = len(
        controller.get_editor_property('RadiantActiveParticles')
    )
output = Path(u.Paths.project_saved_dir()) / 'RadiantDissolve' / 'unified_api.json'
output.write_text(json.dumps(payload, indent=2), encoding='utf-8')
print('RADIANT_UNIFIED_API_TESTED', payload)
