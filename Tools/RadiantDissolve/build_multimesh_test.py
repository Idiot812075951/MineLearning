import unreal as u, json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
from editor_toolset.toolsets.actor import ActorTools as AT
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
E=u.EditorAssetLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.get_current_level().get_outer().get_path_name().startswith(ROOT+'/L_RadiantDissolve_')
bp=E.load_asset(ROOT+'/BP_RadiantTargetSample') or BP.create(ROOT,'BP_RadiantTargetSample',u.Actor.static_class())
for name,shape,loc,scale in [('CylinderBody','Cylinder',u.Vector(0,0,30),u.Vector(.7,.7,.6)),('SphereCap','Sphere',u.Vector(18,0,65),u.Vector(.65,.65,.65))]:
    path=ROOT+'/SM_RadiantSample_'+shape
    mesh=E.load_asset(path) or E.duplicate_asset('/Engine/BasicShapes/'+shape,path)
    mesh.set_editor_property('allow_cpu_access',True)
    settings=mesh.get_editor_property('nanite_settings');settings.enabled=False;mesh.set_editor_property('nanite_settings',settings)
    E.save_loaded_asset(mesh)
    components=AT.get_components(BP.get_default_object(bp))
    component=next((c for c in components if c.get_name().startswith(name)),None) or AT.add_component(bp,u.StaticMeshComponent.static_class(),name)
    component.set_static_mesh(mesh)
    component.set_material(0,E.load_asset(ROOT+'/M_RadiantDissolve_Surface'))
    component.set_editor_property('relative_location',loc)
    component.set_editor_property('relative_scale3d',scale)
    component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    component.set_cast_shadow(False)
BP.compile_blueprint(bp);E.save_loaded_asset(bp)
assert level.get_current_level().get_outer().get_path_name()==ROOT+'/L_RadiantDissolve_Test.L_RadiantDissolve_Test'
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
target=next((a for a in actors.get_all_level_actors() if a.get_actor_label()=='ExternalTargetSample'),None) or actors.spawn_actor_from_class(bp.generated_class(),u.Vector())
target.set_actor_label('ExternalTargetSample')
controller=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='RadiantDissolve_Test')
saved={k:controller.get_editor_property(k) for k in ['PropagationDistance','NoiseStrength','HeatIntensity']}
origin=controller.get_editor_property('DissolveOrigin').get_editor_property('relative_location')
saved['Origin']=[origin.x,origin.y,origin.z]
(Path(u.Paths.project_dir())/'Saved/RadiantDissolve/revision_user_settings.json').write_text(json.dumps(saved,indent=2))
controller.set_editor_property('TargetActor',target)
controller.set_editor_property('PropagationDistance',0.)
controller.set_editor_property('NoiseStrength',12.)
controller.set_editor_property('HeatIntensity',100.)
controller.get_editor_property('DissolveOrigin').set_relative_location(u.Vector(-25,-20,40),False,True)
controller.call_method('Reset')
level.save_current_level()
print('RADIANT_MULTIMESH_READY',len(controller.get_editor_property('BoundMeshes')))
