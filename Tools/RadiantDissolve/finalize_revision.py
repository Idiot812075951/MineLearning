import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
bp=u.load_asset(ROOT+'/BP_RadiantDissolve_Test')
g=BP.get_graph(bp,'PreviewFrame')
if not g:g=BP.add_function_graph(bp,'PreviewFrame')
BP.write_graph_dsl(BP.get_graph(bp,'PreviewFrame'),'''(fn PreviewFrame ()
 (Variables|Default|SetElapsed (Variables|Default|GetPreviewTime))
 (CallFunction|ApplyFrame))''')
BP.set_variable_category(bp,'TargetActor','Target')
for n in ['Duration','PropagationDistance','NoiseStrength','HeatIntensity','DissolveEdgeWidth','ParticleSampleRate']:BP.set_variable_category(bp,n,'Dissolve')
for n in ['PreviewEnabled','PreviewTime']:BP.set_variable_category(bp,n,'Editor Preview')
BP.compile_blueprint(bp)
u.EditorAssetLibrary.save_loaded_asset(bp)
a=next(a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if a.get_actor_label()=='RadiantDissolve_Test')
a.call_method('Reset')
u.EditorAssetLibrary.save_directory(ROOT,only_if_is_dirty=True,recursive=True)
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_REVISION_FINALIZED')
