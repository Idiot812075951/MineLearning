"""PIE regression: real root-motion approaches from arbitrary sides and facings."""
import unreal as u,json,builtins,traceback,math
from pathlib import Path
O=Path(u.Paths.project_saved_dir())/'GurenR19';O.mkdir(exist_ok=True)
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world();p=u.GameplayStatics.get_player_character(w,0);pc=u.GameplayStatics.get_player_controller(w,0)
q=p.get_component_by_class(u.GurenQSkillComponent);mv=p.get_component_by_class(u.CharacterMovementComponent)
p.disable_input(pc)
performance=u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'))
original_throttle=performance.get_editor_property('bThrottleCPUWhenNotForeground')
performance.set_editor_property('bThrottleCPUWhenNotForeground',False)
# Isolate direction/root-motion from obstacle collisions on this test level.
# These are PIE-only actors; the authored course stays intact when PIE stops.
for a in u.GameplayStatics.get_all_actors_of_class(w,u.StaticMeshActor):
    if not a.get_actor_label().startswith('Ground'):
        a.set_actor_enable_collision(False)
u.SystemLibrary.execute_console_command(w,'t.IdleWhenNotForeground 0')
targets=sorted(u.GameplayStatics.get_all_actors_of_class(w,u.QGrabTestDummy),key=lambda a:a.get_name());target=targets[0]
for i,a in enumerate(targets):a.set_actor_location(u.Vector(4500,3000+i*300,298),False,True)
center=u.Vector(0,-1800,298)
cases=[('front_away',-1000,0,180,0),('rear_away',1000,0,0,0),('rear_facing',1000,0,180,0),('left',0,-1000,270,0),('right',0,1000,90,0),('rotated_target',900,500,20,127),('near_rear',180,0,0,0),('far_rear_complete',1900,0,0,0)]
state={'index':-1,'phase':'next'};results=[]
def now():return u.GameplayStatics.get_real_time_seconds(w)
def xyz(v):return [v.x,v.y,v.z]
def dot2(a,b):
    den=math.hypot(a.x,a.y)*math.hypot(b.x,b.y)
    return (a.x*b.x+a.y*b.y)/den if den>1e-6 else 1.
def tick(dt):
    try:
        t=now()
        if state['phase']=='next':
            state['index']+=1
            if state['index']>=len(cases):
                performance.set_editor_property('bThrottleCPUWhenNotForeground',original_throttle)
                p.enable_input(pc);u.unregister_slate_post_tick_callback(builtins.r19_q_direction_tick)
                (O/'q_direction_results.json').write_text(json.dumps(results,indent=2));print('R19_Q_DIRECTION_DONE');return
            name,x,y,yaw,tyaw=cases[state['index']]
            q.cancel();mv.stop_movement_immediately();mv.set_movement_mode(u.MovementMode.MOVE_WALKING)
            target.set_actor_location(center,False,True);target.set_actor_rotation(u.Rotator(pitch=0,yaw=tyaw,roll=0),False)
            p.set_actor_location(u.Vector(center.x+x,center.y+y,232.61),False,True);p.set_actor_rotation(u.Rotator(pitch=0,yaw=yaw,roll=0),False)
            state.update(phase='settle',name=name,at=t,trace=[],dots=[],attached=False,contact_error=None,shot=False)
        elif state['phase']=='settle' and t-state['at']>.7:
            direction=target.get_actor_location()-p.get_actor_location();q.try_cast()
            state.update(phase='run',at=t,start=xyz(p.get_actor_location()),last=p.get_actor_location(),last_stage=q.get_stage(),initial_facing_dot=dot2(p.get_actor_forward_vector(),direction))
        elif state['phase']=='run':
            stage=q.get_stage();loc=p.get_actor_location();delta=loc-state['last'];elapsed=t-state['at']
            if state['last_stage']==u.GurenQStage.DASH and stage==u.GurenQStage.DASH and math.hypot(delta.x,delta.y)>1:
                state['dots'].append(dot2(delta,p.get_actor_forward_vector()))
            state['last']=loc;state['last_stage']=stage
            if not state['trace'] or state['trace'][-1]['stage']!=str(stage):state['trace'].append({'time':elapsed,'stage':str(stage),'location':xyz(loc),'yaw':p.get_actor_rotation().yaw})
            active=q.get_target()
            if active and stage==u.GurenQStage.GRAB:
                if active.get_attach_parent_actor():
                    state['attached']=True
                    if state['name']!='far_rear_complete':q.cancel()
                else:state['contact_error']=(p.mesh.get_socket_location('Q_GrabHead')-active.get_actor_location()).length()
            if not q.is_q_active() or elapsed>9:
                if elapsed>9:q.cancel()
                row={'case':state['name'],'initial_facing_dot':state['initial_facing_dot'],'min_dash_forward_dot':min(state['dots']) if state['dots'] else None,'attached':state['attached'],'last_precontact_error_cm':state['contact_error'],'movement_unlocked':not pc.is_move_input_ignored(),'trace':state['trace'],'timed_out':elapsed>9}
                row['passed']=row['initial_facing_dot']>.999 and (row['min_dash_forward_dot'] is None or row['min_dash_forward_dot']>.5) and row['attached'] and row['movement_unlocked'] and not row['timed_out']
                results.append(row);(O/'q_direction_partial.json').write_text(json.dumps(results,indent=2));state.update(phase='cooldown',at=t)
        elif state['phase']=='cooldown' and t-state['at']>.8:state['phase']='next'
    except Exception:
        performance.set_editor_property('bThrottleCPUWhenNotForeground',original_throttle)
        q.cancel();p.enable_input(pc);u.unregister_slate_post_tick_callback(builtins.r19_q_direction_tick);(O/'q_direction_error.txt').write_text(traceback.format_exc())
builtins.r19_q_direction_tick=u.register_slate_post_tick_callback(tick)
