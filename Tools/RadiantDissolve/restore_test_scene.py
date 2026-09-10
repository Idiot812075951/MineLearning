import unreal as u, json
from pathlib import Path
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.get_current_level().get_outer().get_path_name()==ROOT+'/L_RadiantDissolve_Test.L_RadiantDissolve_Test'
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
a=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='RadiantDissolve_Test')
a.call_method('Reset')
a.set_editor_property('TargetActor',None)
saved=json.loads((Path(u.Paths.project_dir())/'Saved/RadiantDissolve/revision_user_settings.json').read_text())
for key in ['PropagationDistance','NoiseStrength','HeatIntensity']:a.set_editor_property(key,saved[key])
a.get_editor_property('DissolveOrigin').set_relative_location(u.Vector(*saved['Origin']),False,True)
a.call_method('Reset')
sample=next(actor for actor in actors.get_all_level_actors() if actor.get_actor_label()=='ExternalTargetSample')
sample.set_actor_location(u.Vector(350,0,0),False,True)
sample.set_actor_scale3d(u.Vector(.7,.7,.7))
level.save_current_level()
actors.set_selected_level_actors([a])
print('RADIANT_USER_SCENE_RESTORED',saved)
