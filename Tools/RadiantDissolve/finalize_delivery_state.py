"""Leave the test map intact, reset, saved, and ready for P/R testing."""
from pathlib import Path

import unreal as u


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
controller = next(
    actor for actor in actors
    if actor.get_class().get_name() == 'BP_RadiantDissolve_Test_C'
)

controller.set_editor_property('RadiantPreviewEnabled', False)
controller.set_editor_property('RadiantDuration', 2.4)
controller.set_editor_property('RadiantDissolveStart', 0.24)
controller.set_editor_property('RadiantOutlineHoldStart', 0.66)
controller.set_editor_property('RadiantOutlineFadeStart', 0.84)
controller.set_editor_property('RadiantMeshGone', 0.96)
controller.set_editor_property('RadiantParticleRate', 2600.0)
controller.set_editor_property('RadiantParticleFrontWidth', 4.5)
controller.call_method('RadiantReset')

# The hidden built-in AK is a fallback sample.  Keep its serialized slots clean
# even while the placed controller targets the skeletal Gunner sample.
internal_mesh = next(
    (component for component in controller.get_components_by_class(u.StaticMeshComponent)
     if component.get_name().startswith('TargetMesh')),
    None,
)
if internal_mesh and internal_mesh.static_mesh:
    for index, slot in enumerate(internal_mesh.static_mesh.get_editor_property('static_materials')):
        internal_mesh.set_material(index, slot.get_editor_property('material_interface'))

for actor in actors:
    if actor.get_actor_label() == 'SkeletalDissolve_Sample':
        actor.set_actor_hidden_in_game(True)

Path(u.Paths.project_saved_dir(), 'RadiantDissolve', 'phase.txt').write_text('0', encoding='utf-8')
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
u.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
print('RADIANT_DELIVERY_STATE_READY')
