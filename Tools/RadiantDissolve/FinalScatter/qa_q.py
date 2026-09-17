"""Capture the real Q sequence in PIE. No authored camera/map changes."""
import unreal as u,builtins,json,traceback
from pathlib import Path
O=Path(__file__).parent
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
p=u.GameplayStatics.get_player_character(w,0);pc=u.GameplayStatics.get_player_controller(w,0)
q=p.get_component_by_class(u.GurenQSkillComponent);mv=p.get_component_by_class(u.CharacterMovementComponent)
presentation=p.get_component_by_class(u.load_class(None,'/Script/MineLearning.GurenQPresentationComponent'))
old_camera=presentation.get_editor_property('use_execution_camera');presentation.set_editor_property('use_execution_camera',False)
p.disable_input(pc)
perf=u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'))
old_throttle=perf.get_editor_property('bThrottleCPUWhenNotForeground');perf.set_editor_property('bThrottleCPUWhenNotForeground',False)
for a in u.GameplayStatics.get_all_actors_of_class(w,u.StaticMeshActor):
    if not a.get_actor_label().startswith('Ground'):a.set_actor_enable_collision(False)
targets=u.GameplayStatics.get_all_actors_of_class(w,u.QGrabTestDummy);target=targets[0]
for i,a in enumerate(targets):a.set_actor_location(u.Vector(4500,3000+i*300,298),False,True)
target.set_actor_location(u.Vector(0,-1800,298),False,True);target.set_actor_rotation(u.Rotator(),False)
p.set_actor_location(u.Vector(-360,-1800,232.61),False,True);p.set_actor_rotation(u.Rotator(),False);mv.stop_movement_immediately()
cam=p.get_component_by_class(u.CameraComponent);cam.set_absolute(True,True,True);cam.set_editor_property('use_pawn_control_rotation',False);cam.set_field_of_view(48)
cam.set_world_location(u.Vector(680,-1020,540),False,True);cam.set_world_rotation(u.MathLibrary.find_look_at_rotation(cam.get_world_location(),u.Vector(-80,-1800,300)),False,True)
fxclass=u.load_asset('/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test').generated_class()
state={'phase':'warmup','at':u.GameplayStatics.get_real_time_seconds(w),'shots':[], 'samples':[],'slowed':False}
phases=[(.12,'01_sweep'),(.28,'02_full_surface'),(.42,'03_conversion'),(.54,'04_filled_particles'),(.70,'05_scatter'),(.90,'06_fade')]
def finish():
    u.GameplayStatics.set_global_time_dilation(w,1.)
    perf.set_editor_property('bThrottleCPUWhenNotForeground',old_throttle)
    presentation.set_editor_property('use_execution_camera',old_camera)
    p.enable_input(pc);u.unregister_slate_post_tick_callback(builtins.final_scatter_tick)
def shot(label):
    u.SystemLibrary.execute_console_command(w,'HighResShot 1600x1100 filename="'+str(O/(label+'.png')).replace('\\','/')+'"')
def tick(dt):
    try:
        now=u.GameplayStatics.get_real_time_seconds(w)
        if state['phase']=='warmup' and now-state['at']>.8:
            q.try_cast();state.update(phase='run',at=now)
        elif state['phase']=='run':
            stage=q.get_stage()
            if stage==u.GurenQStage.RADIATION:
                if not state['slowed']:
                    u.GameplayStatics.set_global_time_dilation(w,.25);state['slowed']=True
                effects=u.GameplayStatics.get_all_actors_of_class(w,fxclass)
                if effects:
                    fx=effects[0];phase=fx.get_editor_property('RadiantElapsed')/fx.get_editor_property('RadiantDuration')
                    if phases and phase>=phases[0][0]:
                        _,label=phases.pop(0);shot(label);state['shots'].append(label)
                        mesh=target.get_component_by_class(u.SkeletalMeshComponent);mid=mesh.get_material(0)
                        state['samples'].append({'phase':phase,'label':label,'outline':mid.get_scalar_parameter_value('OutlineAmount'),'opacity':mid.get_scalar_parameter_value('OpacityFade'),'fx_rate':[f.get_variable_float('User.SpawnRate') for f in fx.get_editor_property('RadiantActiveParticles')]})
            if not q.is_q_active() or now-state['at']>30:
                state.update(phase='tail',at=now,target_valid=u.SystemLibrary.is_valid(target),movement_unlocked=not pc.is_move_input_ignored())
                u.GameplayStatics.set_global_time_dilation(w,1.)
        elif state['phase']=='tail' and now-state['at']>.4:
            shot('07_empty');(O/'qa_q.json').write_text(json.dumps(state,indent=2));finish();print('FINAL_SCATTER_Q_QA_DONE')
    except Exception:
        (O/'qa_error.txt').write_text(traceback.format_exc());q.cancel();finish()
builtins.final_scatter_tick=u.register_slate_post_tick_callback(tick)
