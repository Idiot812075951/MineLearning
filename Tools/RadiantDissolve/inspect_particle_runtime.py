"""Write compact runtime particle counts for phase-gating QA."""
import json
from pathlib import Path

import unreal as u


world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(
    None,
    '/Game/MineLearning/VFX/RadiantDissolve/'
    'BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C',
)
controllers = u.GameplayStatics.get_all_actors_of_class(world, controller_class)
if not controllers:
    raise RuntimeError('No dissolve controller in PIE')

controller = controllers[0]
rows = []
for component in controller.get_editor_property('RadiantActiveParticles'):
    rows.append({
        'system': component.get_asset().get_name() if component.get_asset() else None,
        'spawn_rate': component.get_variable_float('User.SpawnRate'),
        'particle_speed': component.get_variable_float('User.ParticleSpeed'),
    })

payload = {
    'phase': controller.get_editor_property('RadiantPreviewTime'),
    'particle_start': controller.get_editor_property('RadiantParticleStart'),
    'components': rows,
}
output = Path(u.Paths.project_saved_dir()) / 'RadiantDissolve' / 'particle_runtime.json'
output.write_text(json.dumps(payload, indent=2), encoding='utf-8')
print('RADIANT_PARTICLE_RUNTIME', payload)
