"""Read runtime state of the PIE radiant controller without mutating it."""
import json
import unreal as u

out = u.Paths.project_dir() + 'Saved/RadiantDissolve/pie_state.json'
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(None, '/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C')
controller = u.GameplayStatics.get_all_actors_of_class(world, controller_class)[0]
static_meshes = list(controller.get_editor_property('RadiantStaticMeshes'))
skeletal_meshes = list(controller.get_editor_property('RadiantSkeletalMeshes'))
particles = list(controller.get_editor_property('RadiantActiveParticles'))
material_parameters = []
for mesh in static_meshes + skeletal_meshes:
    slots = []
    for i in range(mesh.get_num_materials()):
        material = mesh.get_material(i)
        values = {}
        if isinstance(material, u.MaterialInstanceDynamic):
            for name in [
                'DissolveProgress', 'HeatRadius', 'DissolveRadius',
                'NoiseStrength', 'HeatIntensity', 'DissolveEdgeWidth',
                'OutlineAmount', 'OpacityFade',
            ]:
                values[name] = material.get_scalar_parameter_value(name)
            values['parent'] = (
                material.parent.get_path_name() if material.parent else None
            )
        slots.append(values)
    material_parameters.append(slots)
report = {
    'elapsed': controller.get_editor_property('RadiantElapsed'),
    'duration': controller.get_editor_property('RadiantDuration'),
    'effective_radius': controller.get_editor_property('RadiantEffectiveRadius'),
    'static_meshes': [mesh.get_path_name() for mesh in static_meshes],
    'skeletal_meshes': [mesh.get_path_name() for mesh in skeletal_meshes],
    'skeletal_visibility': [
        {
            'owner_location': [
                mesh.get_owner().get_actor_location().x,
                mesh.get_owner().get_actor_location().y,
                mesh.get_owner().get_actor_location().z,
            ],
            'component_location': [
                mesh.get_world_location().x,
                mesh.get_world_location().y,
                mesh.get_world_location().z,
            ],
            'visible': mesh.is_visible(),
            'hidden_in_game': mesh.get_editor_property('hidden_in_game'),
            'overlay_material': (
                mesh.get_editor_property('overlay_material').get_path_name()
                if mesh.get_editor_property('overlay_material') else None
            ),
            'owner_hidden': mesh.get_owner().get_editor_property('hidden'),
            'owner_hidden_ed': mesh.get_owner().is_hidden_ed(),
            'owner_temp_hidden_ed': mesh.get_owner().is_temporarily_hidden_in_editor(),
            'owner_hidden_methods': [
                name for name in dir(mesh.get_owner()) if 'hidden' in name.lower()
            ],
        }
        for mesh in skeletal_meshes
    ],
    'particles': [fx.get_path_name() for fx in particles],
    'materials': [
        [mesh.get_material(i).get_path_name() if mesh.get_material(i) else None
         for i in range(mesh.get_num_materials())]
        for mesh in static_meshes + skeletal_meshes
    ],
    'material_parameters': material_parameters,
}
with open(out, 'w', encoding='utf-8') as handle:
    json.dump(report, handle, indent=2)
print('RADIANT_PIE_STATE_WRITTEN')
