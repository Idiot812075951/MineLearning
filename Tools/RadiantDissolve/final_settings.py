import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
from editor_toolset.toolsets.actor import ActorTools as AT
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.get_current_level().get_outer().get_path_name()==ROOT+'/L_RadiantDissolve_Test.L_RadiantDissolve_Test'
bp=u.load_asset(ROOT+'/BP_RadiantDissolve_Test')
# Prototype lighting uses direct lights. Mask animation must not leave stale whole-mesh shadows.
for c in AT.get_components(BP.get_default_object(bp)):
    if isinstance(c,u.StaticMeshComponent):c.set_cast_shadow(False)
BP.compile_blueprint(bp)
u.EditorAssetLibrary.save_loaded_asset(bp)
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='RadiantDissolve_Test':
        a.get_editor_property('TargetMesh').set_cast_shadow(False)
        a.set_editor_property('PreviewTime',0.)
        a.call_method('Reset')
    if a.get_actor_label()=='DemoExposure':
        s=a.get_editor_property('settings')
        for k,v in [('override_dynamic_global_illumination_method',True),('dynamic_global_illumination_method',u.DynamicGlobalIlluminationMethod.NONE),('override_reflection_method',True),('reflection_method',u.ReflectionMethod.NONE),('override_ambient_occlusion_intensity',True),('ambient_occlusion_intensity',0.)]:s.set_editor_property(k,v)
        a.set_editor_property('settings',s)
level.save_current_level()
print('RADIANT_FINAL_SETTINGS_SAVED')
