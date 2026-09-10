"""Place and target the isolated skeletal sample for a PIE verification pass."""
import unreal as u

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()
controller = next(
    actor for actor in all_actors
    if actor.get_class().get_name() == 'BP_RadiantDissolve_Test_C'
    or actor.get_actor_label() in ('RadiantDissolve_Test', 'BP_RadiantDissolve_Test')
)
sample = next(
    (actor for actor in all_actors if actor.get_actor_label() == 'BP_RadiantSkeletalTargetSample'),
    None,
)
if not sample:
    sample = next(
        (actor for actor in all_actors if actor.get_actor_label() == 'SkeletalDissolve_Sample'),
        None,
    )
if not sample:
    sample_bp = u.load_asset(ROOT + '/BP_RadiantSkeletalTargetSample')
    sample = actors.spawn_actor_from_class(sample_bp.generated_class(), u.Vector(0, 0, 0))
    sample.set_actor_label('SkeletalDissolve_Sample')
sample.set_actor_location(u.Vector(-85, 4, -1), False, False)
sample_rotation = u.Rotator()
sample_rotation.yaw = 180.0
sample.set_actor_rotation(sample_rotation, False)
sample.set_actor_scale3d(u.Vector(0.42, 0.42, 0.42))
for other in all_actors:
    if (
        other.get_path_name() != sample.get_path_name()
        and other.get_class().get_name() == 'BP_RadiantSkeletalTargetSample_C'
    ):
        other.set_actor_hidden_in_game(True)
sample.set_actor_hidden_in_game(False)
for component in controller.get_components_by_class(u.StaticMeshComponent):
    if component.get_name().startswith('TargetMesh'):
        component.set_visibility(False, True)
controller.set_editor_property('RadiantTarget', sample)
controller.set_editor_property('RadiantDuration', 30.0)
controller.set_editor_property('RadiantParticleRate', 7200.0)
print('RADIANT_SKELETAL_PIE_TEST_READY')
