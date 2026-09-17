import unreal as u,json
from pathlib import Path
O=Path(u.Paths.project_saved_dir())/'GurenQ'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world();p=u.GameplayStatics.get_player_character(w,0);q=p.get_component_by_class(u.GurenQSkillComponent)
out={'pawn':str(p),'loc':str(p.get_actor_location()),'rotation':str(p.get_actor_rotation()),'components':[c.get_class().get_name() for c in p.get_components_by_class(u.ActorComponent)],'targets':[(a.get_name(),str(a.get_actor_location())) for a in u.GameplayStatics.get_all_actors_of_class(w,u.QGrabTestDummy)]}
(O/'test_initial.json').write_text(json.dumps(out,indent=2))
settings=u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'));settings.set_editor_property('bThrottleCPUWhenNotForeground',False)
u.SystemLibrary.execute_console_command(w,'t.MaxFPS 60')
q.try_cast()
rows=[];elapsed=0.;last=0.
def sample(dt):
 global elapsed,last,q_test_tick
 elapsed+=dt
 if elapsed-last>=.08:
  last=elapsed;t=q.get_target();m=p.get_editor_property('mesh');pres=p.get_component_by_class(u.GurenQPresentationComponent)
  rows.append({'time':round(elapsed,3),'stage':str(q.get_stage()),'pawn':[p.get_actor_location().x,p.get_actor_location().y,p.get_actor_location().z],'target':str(t.get_actor_location()) if t else None,'socket':str(m.get_socket_location('Q_GrabHead')),'montage':str(m.get_anim_instance().get_current_active_montage()),'locked':u.GameplayStatics.get_player_controller(w,0).is_move_input_ignored()})
 if elapsed>6:
  u.unregister_slate_post_tick_callback(q_test_tick);(O/'test_flow.json').write_text(json.dumps(rows,indent=2));print('Q_TEST_DONE')
q_test_tick=u.register_slate_post_tick_callback(sample)
