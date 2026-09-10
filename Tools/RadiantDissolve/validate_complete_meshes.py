"""Validate material routing plus finish/reset visibility for the unified target."""
import json
from pathlib import Path

import unreal as u


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(
    None, ROOT + '/BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C'
)
controller = u.GameplayStatics.get_all_actors_of_class(world, controller_class)[0]
target = controller.get_editor_property('RadiantTarget')


def collected():
    return (
        list(controller.get_editor_property('RadiantStaticMeshes'))
        + list(controller.get_editor_property('RadiantSkeletalMeshes'))
    )


def state(components):
    return [
        {
            'component': component.get_name(),
            'visible': component.is_visible(),
            'materials': [
                {
                    'material': component.get_material(i).get_path_name(),
                    'base': component.get_material(i).get_base_material().get_path_name(),
                }
                for i in range(component.get_num_materials())
            ],
        }
        for component in components
    ]


controller.call_method('RadiantReset')
reset_components = collected()
after_reset = state(reset_components)
controller.call_method(
    'Dissolve',
    args=(target, controller.get_editor_property('DissolveOrigin').get_world_location()),
)
prepared_components = collected()
after_prepare = state(prepared_components)
controller.call_method('RadiantFinish')
after_finish = state(prepared_components)
controller.call_method('RadiantReset')
after_second_reset = state(prepared_components)

payload = {
    'collected_count': len(prepared_components),
    'after_reset': after_reset,
    'after_prepare': after_prepare,
    'after_finish': after_finish,
    'after_second_reset': after_second_reset,
    'all_hidden_at_finish': all(not item['visible'] for item in after_finish),
    'all_visible_after_reset': all(item['visible'] for item in after_second_reset),
}
output = Path(u.Paths.project_saved_dir()) / 'RadiantDissolve' / 'complete_mesh_validation.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(payload, indent=2), encoding='utf-8')
print('RADIANT_COMPLETE_MESH_VALIDATED', payload['all_hidden_at_finish'], payload['all_visible_after_reset'])
