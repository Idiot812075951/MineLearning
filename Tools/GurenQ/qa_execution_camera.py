"""PIE-only camera orbit, shake timing and interruption checks."""
import unreal as u, json, builtins
from pathlib import Path
out = Path(u.Paths.project_saved_dir()) / 'GurenQ'
w = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
p = u.GameplayStatics.get_player_character(w, 0)
pc = u.GameplayStatics.get_player_controller(w, 0)
q = p.get_component_by_class(u.GurenQSkillComponent)
pres = p.get_component_by_class(u.GurenQPresentationComponent)
cam = p.get_component_by_class(u.CameraComponent)
boom = p.get_component_by_class(u.SpringArmComponent)
u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings')).set_editor_property('bThrottleCPUWhenNotForeground', False)
u.SystemLibrary.execute_console_command(w, 't.MaxFPS 60')
targets = sorted(u.GameplayStatics.get_all_actors_of_class(w, u.QGrabTestDummy), key=lambda a:a.get_name())
for i,t in enumerate(targets): t.set_actor_location(u.Vector(4000, -3500+i*1000, 298),False,True)
results=[]
state=dict(index=-1,phase='next')
cases=[('complete',0),('cancel_orbit',1),('cancel_radiation',1)]
def now(): return u.GameplayStatics.get_real_time_seconds(w)
def tick(dt):
    try:
        if state['phase']=='next':
            state['index']+=1
            if state['index']>=len(cases):
                u.unregister_slate_post_tick_callback(builtins.q_exec_tick)
                (out/'qa_execution_camera.json').write_text(json.dumps(results,indent=2))
                print('Q_EXECUTION_CAMERA_QA_DONE');return
            name,index=cases[state['index']]
            targets[index].set_actor_location(u.Vector(-1000,0,298),False,True)
            p.set_actor_location(u.Vector(-1165,0,232.61),False,True)
            pc.set_control_rotation(u.Rotator(pitch=-18,yaw=25))
            boom.set_editor_property('target_arm_length',680)
            boom.set_editor_property('target_offset',u.Vector(5,9,12))
            state.update(phase='settle',name=name,at=now(),trace=[],shake=[],cancelled=False)
        elif state['phase']=='settle' and now()-state['at']>.5:
            q.try_cast();state.update(phase='run',at=now())
        elif state['phase']=='run':
            elapsed=now()-state['at'];stage=q.get_stage();v=cam.get_camera_view(0)
            rotation=pc.get_control_rotation();camera_location=v.get_editor_property('location')
            relative=camera_location-p.get_actor_location()
            forward=p.get_actor_forward_vector()
            front_dot=relative.x*forward.x+relative.y*forward.y
            # Local additive camera rotation isolates shake from the orbital control rotation.
            base_rotation=cam.get_world_rotation();view_rotation=v.get_editor_property('rotation')
            amplitude=sum(abs((getattr(base_rotation,k)-getattr(view_rotation,k)+180)%360-180) for k in ['pitch','yaw','roll'])
            if amplitude>.001: state['shake'].append(dict(time=round(elapsed,3),stage=str(stage),amplitude=amplitude))
            if not state['trace'] or state['trace'][-1]['stage']!=str(stage):
                state['trace'].append(dict(time=round(elapsed,3),stage=str(stage),rotation=str(rotation),front_dot=front_dot,look_locked=pc.is_look_input_ignored()))
            if (state['name']=='cancel_orbit' and elapsed>.45 or state['name']=='cancel_radiation' and stage==u.GurenQStage.RADIATION) and not state['cancelled']:
                state['cancelled']=True;q.cancel()
            if not q.is_q_active() and 'idle_at' not in state:state['idle_at']=now()
            if ('idle_at' in state and now()-state['idle_at']>1.3) or elapsed>9:
                result={k:state[k] for k in ['name','trace','shake','cancelled']}
                result.update(rotation=str(pc.get_control_rotation()),arm=boom.get_editor_property('target_arm_length'),offset=str(boom.get_editor_property('target_offset')),look_unlocked=not pc.is_look_input_ignored(),move_unlocked=not pc.is_move_input_ignored(),scale=u.GameplayStatics.get_global_time_dilation(w),fov=v.get_editor_property('fov'),tick=pres.is_component_tick_enabled())
                results.append(result);(out/'qa_execution_camera_partial.json').write_text(json.dumps(results,indent=2))
                state.pop('idle_at',None);state['phase']='next'
    except Exception as error:
        u.unregister_slate_post_tick_callback(builtins.q_exec_tick);q.cancel()
        (out/'qa_execution_camera_error.txt').write_text(str(error));print('Q_EXEC_QA_ERROR '+str(error))
builtins.q_exec_tick=u.register_slate_post_tick_callback(tick)
