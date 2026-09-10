"""Temporarily preview the controller's built-in multi-slot AK static mesh in PIE."""
import unreal as u

world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(None, '/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C')
controller = u.GameplayStatics.get_all_actors_of_class(world, controller_class)[0]
phase_path = u.Paths.project_saved_dir() + 'RadiantDissolve/phase.txt'
with open(phase_path, 'r', encoding='utf-8') as handle:
    phase = max(0.0, min(1.0, float(handle.read().strip())))
controller.set_editor_property('RadiantTarget', None)
controller.set_editor_property('RadiantDuration', 600.0)
controller.set_editor_property('RadiantPreviewEnabled', False)
controller.set_editor_property('RadiantPreviewTime', phase)
controller.call_method('RadiantPlay')
controller.call_method('RadiantEnablePreview')
player_controller = u.GameplayStatics.get_player_controller(world, 0)
if player_controller:
    controller.disable_input(player_controller)
cameras = u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)
if player_controller and cameras:
    camera = cameras[0]
    focus = controller.get_actor_location() + u.Vector(0.0, 0.0, 38.0)
    camera_location = focus + u.Vector(-110.0, -135.0, 65.0)
    camera.set_actor_location(camera_location, False, False)
    camera.set_actor_rotation(u.MathLibrary.find_look_at_rotation(camera_location, focus), False)
    player_controller.set_view_target_with_blend(camera, 0.0)
print('RADIANT_STATIC_PIE_QA_READY', len(controller.get_editor_property('RadiantActiveParticles')))
