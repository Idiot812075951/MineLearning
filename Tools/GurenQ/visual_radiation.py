import unreal as u,json,builtins
from pathlib import Path
O=Path(u.Paths.project_saved_dir())/'GurenQ';w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world();p=u.GameplayStatics.get_player_character(w,0);pc=u.GameplayStatics.get_player_controller(w,0)
pc.set_control_rotation(u.Rotator(pitch=-12,yaw=-120))
p.get_component_by_class(u.SpringArmComponent).set_editor_property('target_arm_length',750)
u.GameplayStatics.set_global_time_dilation(w,.25)
q=p.get_component_by_class(u.GurenQSkillComponent);q.try_cast()
state={'last':-1,'rows':[],'start':u.GameplayStatics.get_time_seconds(w)}
def visual_sample(dt):
 try:
  elapsed=u.GameplayStatics.get_time_seconds(w)-state['start'];m=p.get_editor_property('mesh');t=q.get_target()
  if elapsed-state['last']>.05:
   state['last']=elapsed;loc=p.get_actor_location();sk=m.get_socket_location('Q_GrabHead')
   state['rows'].append({'time':elapsed,'stage':str(q.get_stage()),'pawn':[loc.x,loc.y,loc.z],'target':str(t.get_actor_location()) if t else None,'grip':[sk.x,sk.y,sk.z],'locked':pc.is_move_input_ignored()})
  if elapsed>2.8:
   u.unregister_slate_post_tick_callback(builtins.q_visual_tick);u.GameplayStatics.set_game_paused(w,True);(O/'visual_radiation.json').write_text(json.dumps(state['rows'],indent=2));print('Q_VISUAL_DONE')
 except Exception as e:
  u.unregister_slate_post_tick_callback(builtins.q_visual_tick);(O/'visual_test_error.txt').write_text(str(e));print(e)
builtins.q_visual_tick=u.register_slate_post_tick_callback(visual_sample)
