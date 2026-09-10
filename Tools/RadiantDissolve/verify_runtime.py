"""Exercise the shipped Blueprint functions in PIE, including interrupted playback."""
import unreal as u, time, json
from pathlib import Path
w=u.EditorLevelLibrary.get_pie_worlds(False)[0]
assert 'L_RadiantDissolve_Test' in w.get_path_name()
a=next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.Actor) if a.get_actor_label()=='RadiantDissolve_Test')
fx=a.get_editor_property('DissolveParticles')
fx.set_paused(False)
out=Path(u.Paths.project_dir())/'Saved/RadiantDissolve'
results=[]
start=time.monotonic()
index=0
events=[(.22,'heat'),(.8,'front'),(2.45,'complete'),(2.6,'replay'),(3.3,'interrupt_reset'),(3.5,'reset_check'),(3.7,'replay'),(6.2,'complete_again'),(6.4,'origin_check'),(7.0,'restore')]
a.call_method('TestDissolve')
origin=a.get_editor_property('DissolveOrigin')
location=origin.get_editor_property('relative_location')
saved_origin=u.Vector(location.x,location.y,location.z)

def sample(label):
    mid=a.get_editor_property('DynamicMaterial')
    count=0
    if fx.is_active():
        c=u.NiagaraSimCacheFunctionLibrary.capture_niagara_sim_cache_immediate(u.NiagaraSimCache(),u.NiagaraSimCacheCreateParameters(),fx,False,.016)
        if c:count=len(c.read_position_attribute('Position','SurfaceEnergy'))
    result={'label':label,'elapsed':a.get_editor_property('Elapsed'),'heat':mid.get_scalar_parameter_value('HeatRadius'),'dissolve':mid.get_scalar_parameter_value('DissolveRadius'),'particle_count':count,'fx_active':fx.is_active()}
    results.append(result)
    return result

def tick(delta):
    global index,handle
    if index>=len(events):return
    when,kind=events[index]
    if time.monotonic()-start<when:return
    index+=1
    try:
        if kind=='replay':a.call_method('TestDissolve')
        elif kind=='interrupt_reset':a.call_method('Reset')
        elif kind=='origin_check':
            a.call_method('Reset')
            origin.set_relative_location(u.Vector(-20,38,32),False,True)
            a.set_editor_property('PreviewTime',.65);a.call_method('UserConstructionScript')
            mid=a.get_editor_property('DynamicMaterial')
            color=mid.get_vector_parameter_value('DissolveOriginWS')
            results.append({'label':kind,'material_origin':[color.r,color.g,color.b],'scene_origin':str(origin.get_world_location())})
            u.AutomationLibrary.take_high_res_screenshot(960,540,str(out/'alternate_origin.png'))
        elif kind=='restore':
            origin.set_relative_location(saved_origin,False,True)
            a.set_editor_property('PreviewTime',0.)
            a.call_method('Reset')
            sample('final_reset')
            u.unregister_slate_post_tick_callback(handle)
            (out/'runtime_validation.json').write_text(json.dumps(results,indent=2))
            print('RADIANT_RUNTIME_VERIFIED',json.dumps(results))
        else:sample(kind)
    except Exception as error:
        u.unregister_slate_post_tick_callback(handle)
        (out/'runtime_validation_error.txt').write_text(str(error))
        raise

handle=u.register_slate_post_tick_callback(tick)

