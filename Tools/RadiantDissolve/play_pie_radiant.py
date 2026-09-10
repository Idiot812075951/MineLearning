"""Trigger a slowed PIE pass so MCP capture latency still lands during the effect."""
import unreal as u

world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
if not world:
    raise RuntimeError('No PIE game world')
controller_class = u.load_class(None, '/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C')
controller = u.GameplayStatics.get_all_actors_of_class(world, controller_class)[0]
# Slow only the current PIE instance so MCP capture can land inside the effect.
controller.set_editor_property('RadiantDuration', 120.0)
controller.set_editor_property('RadiantParticleRate', 7200.0)
controller.call_method('RadiantPlay')
print('RADIANT_PIE_PLAY_STARTED')
