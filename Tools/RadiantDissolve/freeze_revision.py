import unreal as u
w=u.EditorLevelLibrary.get_pie_worlds(False)[0]
a=next(a for a in u.GameplayStatics.get_all_actors_of_class(w,u.Actor) if a.get_actor_label()=='RadiantDissolve_Test')
a.call_method('Reset')
a.set_editor_property('NoiseStrength',12.)
a.set_editor_property('PropagationDistance',0.)
a.call_method('PrepareMaterials')
a.call_method('SpawnParticles')
effects=a.get_editor_property('ActiveParticles')
for fx in effects:fx.set_paused(True)
for i in range(1,23):
    a.set_editor_property('PreviewTime',i/24.)
    a.call_method('PreviewFrame')
    for fx in effects:
        fx.set_paused(False)
        fx.advance_simulation(1,1/24.)
        fx.set_paused(True)
print('FROZEN_FRONT',a.get_editor_property('Elapsed'))
