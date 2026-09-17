"""Real-time Q: cancel at conversion, then run again with the authored camera."""
import unreal as u,builtins,json,traceback
from pathlib import Path
O=Path(__file__).parent
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
p=u.GameplayStatics.get_player_character(w,0);pc=u.GameplayStatics.get_player_controller(w,0)
q=p.get_component_by_class(u.GurenQSkillComponent);mv=p.get_component_by_class(u.CharacterMovementComponent)
perf=u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'))
old=perf.get_editor_property('bThrottleCPUWhenNotForeground');perf.set_editor_property('bThrottleCPUWhenNotForeground',False)
p.disable_input(pc)
targets=u.GameplayStatics.get_all_actors_of_class(w,u.QGrabTestDummy);target=targets[0]
for i,a in enumerate(targets):a.set_actor_location(u.Vector(4500,3000+i*300,298),False,True)
for a in u.GameplayStatics.get_all_actors_of_class(w,u.StaticMeshActor):
    if not a.get_actor_label().startswith('Ground'):a.set_actor_enable_collision(False)
fxclass=u.load_asset('/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test').generated_class()
mesh=target.get_component_by_class(u.SkeletalMeshComponent)
original=[mesh.get_material(i) for i in range(mesh.get_num_materials())]
state={'phase':'setup','at':u.GameplayStatics.get_real_time_seconds(w),'round':0,'shot':False};report={}
def finish():
    p.enable_input(pc);perf.set_editor_property('bThrottleCPUWhenNotForeground',old)
    u.unregister_slate_post_tick_callback(builtins.final_lifecycle_tick)
def tick(dt):
    try:
        t=u.GameplayStatics.get_real_time_seconds(w)
        if state['phase']=='setup':
            target.set_actor_location(u.Vector(0,-1800,298),False,True);target.set_actor_rotation(u.Rotator(),False)
            p.set_actor_location(u.Vector(-380,-1800,232.61),False,True);p.set_actor_rotation(u.Rotator(),False);mv.stop_movement_immediately()
            state.update(phase='settle',at=t)
        elif state['phase']=='settle' and t-state['at']>.6:
            q.try_cast();state.update(phase='run',at=t)
        elif state['phase']=='run':
            fx=u.GameplayStatics.get_all_actors_of_class(w,fxclass)
            phase=fx[0].get_editor_property('RadiantElapsed')/fx[0].get_editor_property('RadiantDuration') if fx else -1
            if state['round']==0 and phase>.48:
                q.cancel();state.update(phase='cancel_check',at=t)
            elif state['round']==1:
                if phase>.54 and not state['shot']:
                    u.SystemLibrary.execute_console_command(w,'HighResShot 1600x1100 filename="'+str(O/'08_authored_camera.png').replace('\\','/')+'"');state['shot']=True
                if not q.is_q_active():
                    state.update(phase='done',at=t)
            if t-state['at']>15:raise RuntimeError('Q lifecycle timeout')
        elif state['phase']=='cancel_check' and t-state['at']>.3:
            report['cancel']={'target_valid':u.SystemLibrary.is_valid(target),'visible':mesh.is_visible(),'exact_materials_restored':[mesh.get_material(i)==original[i] for i in range(len(original))],'fx_actors':len(u.GameplayStatics.get_all_actors_of_class(w,fxclass)),'movement_unlocked':not pc.is_move_input_ignored()}
            assert all(report['cancel']['exact_materials_restored']) and report['cancel']['visible'] and report['cancel']['fx_actors']==0
            state.update(phase='setup',round=1,at=t)
        elif state['phase']=='done' and t-state['at']>.5:
            report['repeat']={'target_destroyed':not u.SystemLibrary.is_valid(target),'fx_actors':len(u.GameplayStatics.get_all_actors_of_class(w,fxclass)),'movement_unlocked':not pc.is_move_input_ignored(),'default_camera_capture':state['shot']}
            assert report['repeat']['target_destroyed'] and report['repeat']['fx_actors']==0 and report['repeat']['movement_unlocked']
            (O/'qa_lifecycle.json').write_text(json.dumps(report,indent=2));finish();print('FINAL_SCATTER_LIFECYCLE_PASS')
    except Exception:
        (O/'qa_lifecycle_error.txt').write_text(traceback.format_exc());q.cancel();finish()
builtins.final_lifecycle_tick=u.register_slate_post_tick_callback(tick)
