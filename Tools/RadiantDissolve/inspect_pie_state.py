"""Capture compact runtime state for the dissolve controller and skeletal samples."""
import json
from pathlib import Path

import unreal as u


world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(
    None,
    '/Game/MineLearning/VFX/RadiantDissolve/'
    'BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C',
)
controllers = u.GameplayStatics.get_all_actors_of_class(world, controller_class) if world else []
controller = controllers[0] if controllers else None
payload = {
    'controller': controller.get_path_name() if controller else None,
    'target': None,
    'phase': None,
    'static_mesh_count': 0,
    'skeletal_mesh_count': 0,
    'using_skeletal_fade': None,
    'particles': [],
    'materials': [],
    'static_materials': [],
    'actors': [],
}
if controller:
    target = controller.get_editor_property('RadiantTarget')
    payload['target'] = target.get_path_name() if target else None
    payload['phase'] = controller.get_editor_property('RadiantPreviewTime')
    payload['static_mesh_count'] = len(controller.get_editor_property('RadiantStaticMeshes'))
    payload['skeletal_mesh_count'] = len(controller.get_editor_property('RadiantSkeletalMeshes'))
    payload['using_skeletal_fade'] = controller.get_editor_property('RadiantUsingSkeletalFadeMaterial')
    for particle in controller.get_editor_property('RadiantActiveParticles'):
        payload['particles'].append({
            'path': particle.get_path_name(),
            'active': particle.is_active(),
            'system': particle.get_asset().get_path_name() if particle.get_asset() else None,
        })
    for mesh in controller.get_editor_property('RadiantSkeletalMeshes'):
        for slot in range(mesh.get_num_materials()):
            material = mesh.get_material(slot)
            row = {'path': material.get_path_name() if material else None}
            if isinstance(material, u.MaterialInstanceDynamic):
                for parameter in ('DissolveProgress', 'HeatRadius', 'DissolveRadius', 'OutlineAmount', 'OpacityFade'):
                    row[parameter] = material.get_scalar_parameter_value(parameter)
            payload['materials'].append(row)
    for mesh in controller.get_editor_property('RadiantStaticMeshes'):
        for slot in range(mesh.get_num_materials()):
            material = mesh.get_material(slot)
            row = {'path': material.get_path_name() if material else None}
            if isinstance(material, u.MaterialInstanceDynamic):
                row['base_material'] = material.get_base_material().get_path_name()
                for parameter in ('DissolveProgress', 'HeatRadius', 'DissolveRadius', 'OutlineAmount', 'OpacityFade'):
                    row[parameter] = material.get_scalar_parameter_value(parameter)
            payload['static_materials'].append(row)

for actor in u.GameplayStatics.get_all_actors_of_class(world, u.Actor) if world else []:
    if 'Radiant' not in actor.get_name() and 'SkeletalTarget' not in actor.get_name():
        continue
    payload['actors'].append({
        'name': actor.get_name(),
        'label': actor.get_actor_label(),
        'hidden_in_game': actor.get_editor_property('hidden'),
        'location': {
            'x': actor.get_actor_location().x,
            'y': actor.get_actor_location().y,
            'z': actor.get_actor_location().z,
        },
    })

output = Path(u.Paths.project_saved_dir()) / 'RadiantDissolve' / 'pie_state.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(payload, indent=2), encoding='utf-8')
print('RADIANT_PIE_STATE_INSPECTED')
