import unreal as u, time, json
from pathlib import Path
w=u.EditorLevelLibrary.get_pie_worlds(False)[0]
a=next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.Actor) if a.get_actor_label()=='RadiantDissolve_Test')
out=Path(u.Paths.project_dir())/'Saved/RadiantDissolve/Revision'
out.mkdir(parents=True,exist_ok=True)
records=[]
start=u.GameplayStatics.get_time_seconds(w)
index=0
events=[(0,'idle'),(.2,'play'),(.75,'heat'),(1.2,'front'),(1.85,'residue'),(3.1,'complete'),(3.2,'replay'),(3.6,'reset'),(3.8,'reset_check')]
a.call_method('Reset')

def sample(label):
    meshes=a.get_editor_property('BoundMeshes')
    materials=[m.get_material(i) for m in meshes for i in range(m.get_num_materials())]
    particles=[]
    for fx in a.get_editor_property('ActiveParticles'):
        cache=u.NiagaraSimCacheFunctionLibrary.capture_niagara_sim_cache_immediate(u.NiagaraSimCache(),u.NiagaraSimCacheCreateParameters(),fx,False,.016)
        if cache:
            positions=cache.read_position_attribute('Position','SurfaceEnergy')
            particles.append(len(positions))
    record={'label':label,'elapsed':a.get_editor_property('Elapsed'),'meshes':len(meshes),'progress':[m.get_scalar_parameter_value('DissolveProgress') for m in materials],'particle_count':sum(particles)}
    records.append(record)
    # Screenshots pause PIE time; capture visuals separately from runtime timing.

def tick(delta):
    global index,handle
    if index>=len(events):return
    when,label=events[index]
    if u.GameplayStatics.get_time_seconds(w)-start<when:return
    index+=1
    try:
        if label in ['play','replay']:a.call_method('TestDissolve')
        elif label=='reset':a.call_method('Reset')
        else:sample(label)
        if index==len(events):
            u.unregister_slate_post_tick_callback(handle)
            (out/'verification.json').write_text(json.dumps(records,indent=2))
            print('RADIANT_REVISION_VERIFIED',json.dumps(records))
    except Exception as error:
        u.unregister_slate_post_tick_callback(handle)
        print('RADIANT_REVISION_FAILED',str(error))
        raise
handle=u.register_slate_post_tick_callback(tick)
