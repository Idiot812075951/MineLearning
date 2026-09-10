"""Deterministic 24 fps UE viewport capture; run while the independent demo is in PIE."""
import unreal as u, json
from pathlib import Path
world=u.EditorLevelLibrary.get_pie_worlds(False)[0]
camera=u.GameplayStatics.get_player_controller(world,0).get_view_target()
assert 'L_RadiantDissolve_Test' in world.get_path_name()
demo=next(a for a in u.GameplayStatics.get_all_actors_of_class(world,u.Actor) if a.get_actor_label()=='RadiantDissolve_Test')
output=Path(u.Paths.project_dir())/'Saved/RadiantDissolve/Preview'
output.mkdir(parents=True,exist_ok=True)
demo.call_method('Reset')
demo.call_method('PrepareMaterials')
demo.call_method('SpawnParticles')
effects=demo.get_editor_property('ActiveParticles')
for fx in effects:fx.set_paused(True)
frames=72
frame_index=0
task=None
wait_ticks=0
records=[]

def capture_tick(delta):
    global frame_index,task,wait_ticks,capture_handle
    try:
        if wait_ticks>0:
            wait_ticks-=1
            return
        if task and not task.is_task_done():return
        if frame_index>=frames:
            demo.call_method('Reset')
            (output/'frames.json').write_text(json.dumps(records,indent=2))
            u.unregister_slate_post_tick_callback(capture_handle)
            print('RADIANT_CAPTURE_COMPLETE',frames)
            return
        t=max(0,(frame_index-8)/24.)
        demo.set_editor_property('PreviewTime',t)
        demo.call_method('PreviewFrame')
        for fx in effects:
            fx.set_variable_float('User.SpawnRate',demo.get_editor_property('ParticleSampleRate')/len(effects) if 0<t<demo.get_editor_property('Duration') else 0.)
            fx.set_paused(False)
            fx.advance_simulation(1,1/24.)
            fx.set_paused(True)
        mid=demo.get_editor_property('BoundMeshes')[0].get_material(0)
        records.append({'frame':frame_index,'seconds':t,'heat':mid.get_scalar_parameter_value('HeatRadius'),'dissolve':mid.get_scalar_parameter_value('DissolveRadius')})
        task=u.AutomationLibrary.take_high_res_screenshot(960,540,str(output/f'frame_{frame_index:03d}.png'),camera=camera)
        frame_index+=1
        wait_ticks=3
    except Exception as error:
        u.unregister_slate_post_tick_callback(capture_handle)
        for fx in effects:fx.set_paused(False)
        print('RADIANT_CAPTURE_ERROR',str(error))
        raise

capture_handle=u.register_slate_post_tick_callback(capture_tick)
print('RADIANT_CAPTURE_STARTED')

