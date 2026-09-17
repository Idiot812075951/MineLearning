"""PIE-only acceptance checks; no persistent world changes."""
import unreal as u,json,builtins
from pathlib import Path
O=Path(u.Paths.project_saved_dir())/'GurenQ';w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world();p=u.GameplayStatics.get_player_character(w,0);pc=u.GameplayStatics.get_player_controller(w,0);q=p.get_component_by_class(u.GurenQSkillComponent);m=p.get_editor_property('mesh');mv=p.get_component_by_class(u.CharacterMovementComponent)
u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings')).set_editor_property('bThrottleCPUWhenNotForeground',False)
targets=sorted(u.GameplayStatics.get_all_actors_of_class(w,u.QGrabTestDummy),key=lambda a:a.get_name())
cases=[('empty',None,600),('flying',0,350),('interrupt_radiation',0,480),('far_low_fps',0,350),('near',1,165),('destroy_target',2,350)]
results=[];state={'i':-1,'phase':'next','at':u.GameplayStatics.get_time_seconds(w),'trace':[],'once':False}
def now():return u.GameplayStatics.get_time_seconds(w)
def qa_tick(dt):
 try:
  if state['phase']=='next':
   state['i']+=1
   if state['i']>=len(cases):
    u.SystemLibrary.execute_console_command(w,'t.MaxFPS 60');u.unregister_slate_post_tick_callback(builtins.q_cases_tick);(O/'qa_results.json').write_text(json.dumps(results,indent=2));print('Q_CASES_DONE');return
   name,idx,dist=cases[state['i']];state.update(name=name,phase='settle',at=now(),trace=[],once=False)
   u.SystemLibrary.execute_console_command(w,'t.MaxFPS '+('5' if name=='far_low_fps' else '60'))
   q.cancel();mv.stop_movement_immediately();mv.set_movement_mode(u.MovementMode.MOVE_WALKING)
   loc=targets[idx].get_actor_location() if idx is not None else u.Vector(-5000,-3000,0)
   p.set_actor_location(u.Vector(loc.x-dist,loc.y,232.61),False,True);p.set_actor_rotation(u.Rotator(),True)
   state['original_material']=str(targets[idx].get_editor_property('body').get_material(0)) if idx is not None else ''
  elif state['phase']=='settle' and now()-state['at']>.5:
   if state['name']=='flying':mv.set_movement_mode(u.MovementMode.MOVE_FLYING)
   q.try_cast();state['started']=str(q.get_stage());state['phase']='run';state['at']=now()
  elif state['phase']=='run':
   stage=str(q.get_stage());elapsed=now()-state['at'];t=q.get_target();name=state['name'];loc=p.get_actor_location()
   if not state['trace'] or state['trace'][-1]['stage']!=stage:
    state['trace'].append({'time':round(elapsed,3),'stage':stage,'location':[loc.x,loc.y,loc.z]})
   if name=='interrupt_radiation' and q.get_stage()==u.GurenQStage.RADIATION and not state['once']:
    state['once']=True;state['material_during_radiation']=str(t.get_editor_property('body').get_material(0));m.get_anim_instance().montage_stop(.1,m.get_anim_instance().get_current_active_montage())
   if name=='destroy_target' and q.get_stage()==u.GurenQStage.GRAB and elapsed>1 and not state['once']:
    state['once']=True;t.destroy_actor()
   if name=='far_low_fps' and q.get_stage()==u.GurenQStage.GRAB and not state['once']:
    state['once']=True;before=m.get_anim_instance().get_current_active_montage();q.try_cast();p.call_method('TryAttack');p.jump();state['attack_blocked']=before==m.get_anim_instance().get_current_active_montage();state['flight_blocked']=not mv.is_flying()
   if (not q.is_q_active() and elapsed>.7) or elapsed>8.5:
    result={'case':name,'initial_stage':state['started'],'trace':state['trace'],'unlocked':not pc.is_move_input_ignored(),'idle':not q.is_q_active(),'remaining_targets':len(u.GameplayStatics.get_all_actors_of_class(w,u.QGrabTestDummy))}
    for k in ['material_during_radiation','attack_blocked','flight_blocked']:
     if k in state:result[k]=state.pop(k)
    if name=='interrupt_radiation':result['material_restored']=str(targets[0].get_editor_property('body').get_material(0))==state['original_material'];result['detached']=targets[0].get_attach_parent_actor() is None
    results.append(result);(O/'qa_results_partial.json').write_text(json.dumps(results,indent=2));state['phase']='next'
 except Exception as e:
  u.unregister_slate_post_tick_callback(builtins.q_cases_tick);q.cancel();u.SystemLibrary.execute_console_command(w,'t.MaxFPS 60');(O/'qa_error.txt').write_text(str(e));print('Q_QA_ERROR '+str(e))
builtins.q_cases_tick=u.register_slate_post_tick_callback(qa_tick)
