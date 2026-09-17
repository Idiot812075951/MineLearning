"""Save only Q delivery assets after PIE; restore temporary QA editor settings."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as B

root = '/Game/MineLearning/Characters/Guren'
for name in ['BPC_GurenQPresentation', 'BP_QGrabTestDummy', 'BP_GurenRetargetTest', 'ABP_GurenLocomotion']:
    bp = u.load_asset(root + '/Blueprints/' + name)
    B.compile_blueprint(bp)
    u.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=True)
for path in u.EditorAssetLibrary.list_assets(root + '/Animations/Q', recursive=True):
    u.EditorAssetLibrary.save_asset(path, only_if_is_dirty=True)
u.EditorAssetLibrary.save_asset(root + '/Skeletal/SK_Guren_Skeleton', only_if_is_dirty=True)
u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings')).set_editor_property('bThrottleCPUWhenNotForeground', True)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world, 't.MaxFPS 0')
print('Q_DELIVERY_SAVED')
