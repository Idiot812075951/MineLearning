import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
from editor_toolset.toolsets.actor import ActorTools as AT
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
E=u.EditorAssetLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.get_current_level().get_outer().get_path_name()==ROOT+'/L_RadiantDissolve_Test.L_RadiantDissolve_Test'
# GameModeBase otherwise creates a visible DefaultPawn sphere inside the test ore.
gm=E.load_asset(ROOT+'/BP_RadiantDemoGameMode') if E.does_asset_exist(ROOT+'/BP_RadiantDemoGameMode') else BP.create(ROOT,'BP_RadiantDemoGameMode',u.GameModeBase.static_class())
BP.compile_blueprint(gm)
defaults=BP.get_default_object(gm)
defaults.set_editor_property('default_pawn_class',u.SpectatorPawn.static_class())
defaults.set_editor_property('hud_class',None)
BP.compile_blueprint(gm)
E.save_loaded_asset(gm)
level.get_current_level().get_outer().get_world_settings().set_editor_property('default_game_mode',gm.generated_class())
bp=E.load_asset(ROOT+'/BP_RadiantDissolve_Test')
for c in AT.get_components(BP.get_default_object(bp)):
    if isinstance(c,u.StaticMeshComponent):
        c.set_editor_property('shadow_cache_invalidation_behavior',u.ShadowCacheInvalidationBehavior.ALWAYS)
        c.set_editor_property('affect_distance_field_lighting',False)
        c.set_editor_property('visible_in_ray_tracing',False)
BP.compile_blueprint(bp);E.save_loaded_asset(bp)
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='RadiantDissolve_Test':
        c=a.get_editor_property('TargetMesh')
        c.set_editor_property('shadow_cache_invalidation_behavior',u.ShadowCacheInvalidationBehavior.ALWAYS)
        c.set_editor_property('affect_distance_field_lighting',False)
        c.set_editor_property('visible_in_ray_tracing',False)
        a.set_editor_property('PreviewTime',0.);a.call_method('Reset')
level.save_current_level()
print('RADIANT_SCENE_FINALIZED')
