"""Set the running PIE dissolve to an exact normalized phase for visual QA."""
import unreal as u

phase_path = u.Paths.project_saved_dir() + 'RadiantDissolve/phase.txt'
with open(phase_path, 'r', encoding='utf-8') as handle:
    phase = max(0.0, min(1.0, float(handle.read().strip())))

world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
if not world:
    raise RuntimeError('No PIE game world')

controller_class = u.load_class(
    None,
    '/Game/MineLearning/VFX/RadiantDissolve/'
    'BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C',
)
controller = u.GameplayStatics.get_all_actors_of_class(world, controller_class)[0]
controller.call_method('RadiantPreviewAt', args=(phase,))
print(f'RADIANT_PIE_PHASE={phase:.3f}')
