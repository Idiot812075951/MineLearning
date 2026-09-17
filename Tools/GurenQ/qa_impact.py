"""PIE-only long dash and presentation restoration checks."""
import unreal as u, json, builtins
from pathlib import Path

out = Path(u.Paths.project_saved_dir()) / 'GurenQ'
w = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
p = u.GameplayStatics.get_player_character(w, 0)
pc = u.GameplayStatics.get_player_controller(w, 0)
q = p.get_component_by_class(u.GurenQSkillComponent)
present = p.get_component_by_class(u.GurenQPresentationComponent)
camera = p.get_component_by_class(u.CameraComponent)
targets = sorted(u.GameplayStatics.get_all_actors_of_class(w, u.QGrabTestDummy), key=lambda a: a.get_name())
u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings')).set_editor_property('bThrottleCPUWhenNotForeground', False)
u.SystemLibrary.execute_console_command(w, 't.MaxFPS 60')
for i, target in enumerate(targets):
    target.set_actor_location(u.Vector(4000, -3500 + i * 1000, 298), False, True)
cases = [('long1000', 1000, 0), ('long2000', 2000, 1), ('outside2001', 2001, 2), ('cancel_slowmo_near', 165, 2)]
results = []
state = dict(index=-1, phase='next')

def now():
    return u.GameplayStatics.get_real_time_seconds(w)

def sample(dt):
    try:
        if state['phase'] == 'next':
            state['index'] += 1
            if state['index'] == len(cases):
                u.unregister_slate_post_tick_callback(builtins.q_impact_tick)
                (out / 'qa_impact.json').write_text(json.dumps(results, indent=2))
                print('Q_IMPACT_QA_DONE')
                return
            name, distance, index = cases[state['index']]
            target = targets[index]
            target.set_actor_location(u.Vector(-1000, 0, 298), False, True)
            p.set_actor_location(u.Vector(-1000-distance, 0, 232.61), False, True)
            p.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            state.update(phase='settle', at=now(), name=name, target=target, trace=[], min_scale=1., max_ghosts=0, min_fov=90., max_fov=90., cancelled=False)
        elif state['phase'] == 'settle' and now()-state['at'] > .5:
            state['base_fov'] = camera.get_editor_property('field_of_view')
            q.try_cast()
            state.update(phase='run', at=now())
        elif state['phase'] == 'run':
            elapsed = now()-state['at']
            stage = str(q.get_stage())
            scale = u.GameplayStatics.get_global_time_dilation(w)
            ghosts = list(p.get_components_by_class(u.PoseableMeshComponent))
            view = camera.get_camera_view(0.)
            fov = view.get_editor_property('fov')
            state['min_fov'] = min(state['min_fov'], fov)
            state['max_fov'] = max(state['max_fov'], fov)
            state['min_scale'] = min(state['min_scale'], scale)
            state['max_ghosts'] = max(state['max_ghosts'], len(ghosts))
            if not state['trace'] or state['trace'][-1]['stage'] != stage:
                state['trace'].append(dict(time=round(elapsed,3), stage=stage, location=str(p.get_actor_location())))
            if state['name'] == 'cancel_slowmo_near' and scale < .99 and not state['cancelled']:
                state['cancelled'] = True
                q.cancel()
            if not q.is_q_active() and 'idle_at' not in state:
                state['idle_at'] = now()
            if ('idle_at' in state and now()-state['idle_at'] > 1.2) or elapsed > 10:
                result = {key:state[key] for key in ['name','trace','min_scale','max_ghosts','min_fov','max_fov','cancelled']}
                result.update(idle=not q.is_q_active(), unlocked=not pc.is_move_input_ignored(), time_scale=u.GameplayStatics.get_global_time_dilation(w), final_fov=fov, remaining_ghosts=len(ghosts), presentation_tick=present.is_component_tick_enabled())
                if state['name'].startswith('cancel'):
                    result['detached'] = state['target'].get_attach_parent_actor() is None
                results.append(result)
                (out / 'qa_impact_partial.json').write_text(json.dumps(results, indent=2))
                state.pop('idle_at', None)
                state['phase'] = 'next'
    except Exception as error:
        u.unregister_slate_post_tick_callback(builtins.q_impact_tick)
        q.cancel()
        (out / 'qa_impact_error.txt').write_text(str(error))
        print('Q_IMPACT_QA_ERROR ' + str(error))

builtins.q_impact_tick = u.register_slate_post_tick_callback(sample)
