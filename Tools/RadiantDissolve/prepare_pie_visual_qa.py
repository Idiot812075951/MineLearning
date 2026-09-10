"""Clear the runtime stage around the active target for close visual QA only."""
import unreal as u


world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(
    None,
    '/Game/MineLearning/VFX/RadiantDissolve/'
    'BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C',
)
controllers = u.GameplayStatics.get_all_actors_of_class(world, controller_class)
if not controllers:
    raise RuntimeError('No PIE dissolve controller')
controller = controllers[0]
target = controller.get_editor_property('RadiantTarget')
phase_path = u.Paths.project_saved_dir() + 'RadiantDissolve/phase.txt'
with open(phase_path, 'r', encoding='utf-8') as handle:
    phase = max(0.0, min(1.0, float(handle.read().strip())))
controller.set_editor_property('RadiantDuration', 600.0)
controller.call_method('RadiantPlay')
print('RADIANT_QA_COUNT_AFTER_PLAY', len(controller.get_editor_property('RadiantActiveParticles')))
if len(controller.get_editor_property('RadiantActiveParticles')) == 0:
    raise RuntimeError('RadiantPlay did not create Niagara components')
controller.call_method('RadiantPreviewAt', args=(phase,))
print('RADIANT_QA_COUNT_AFTER_PREVIEW', len(controller.get_editor_property('RadiantActiveParticles')))
for actor in u.GameplayStatics.get_all_actors_of_class(world, u.Actor):
    if actor != target and actor.get_actor_label() in ('ExternalTargetSample', 'SkeletalDissolve_Sample'):
        actor.set_actor_hidden_in_game(True)
if target:
    target.set_actor_location(u.Vector(-15.0, 0.0, 0.0), False, False)
print('RADIANT_QA_COUNT_AFTER_MOVE', len(controller.get_editor_property('RadiantActiveParticles')))
player_controller = u.GameplayStatics.get_player_controller(world, 0)
if player_controller:
    # Prevent the editor command text used by automated QA from also firing the
    # actor's raw P/R test shortcuts while PIE is ejected.
    controller.disable_input(player_controller)
print('RADIANT_QA_COUNT_AFTER_DISABLE', len(controller.get_editor_property('RadiantActiveParticles')))
cameras = u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)
if player_controller and cameras:
    camera = cameras[0]
    camera_location = u.Vector(-82.0, -98.0, 92.0)
    camera.set_actor_location(camera_location, False, False)
    look_at = u.MathLibrary.find_look_at_rotation(
        camera_location,
        target.get_actor_location() + u.Vector(0.0, 0.0, 42.0),
    )
    camera.set_actor_rotation(look_at, False)
    player_controller.set_view_target_with_blend(camera, 0.0)
print('RADIANT_QA_COUNT_AFTER_CAMERA', len(controller.get_editor_property('RadiantActiveParticles')))
controller.call_method('RadiantPlaceOrigin')
print('RADIANT_QA_COUNT_AFTER_ORIGIN', len(controller.get_editor_property('RadiantActiveParticles')))
controller.call_method('RadiantComputeCoverage')
print('RADIANT_QA_COUNT_AFTER_COVERAGE', len(controller.get_editor_property('RadiantActiveParticles')))
controller.call_method('RadiantApplyFrame')
print('RADIANT_QA_COUNT_AFTER_FRAME', len(controller.get_editor_property('RadiantActiveParticles')))
print('RADIANT_PIE_VISUAL_QA_READY')
