"""Inspect every skeletal sample in the current PIE world."""
import json
import unreal as u

world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
sample_class = u.load_class(
    None,
    '/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantSkeletalTargetSample.BP_RadiantSkeletalTargetSample_C',
)
rows = []
for sample in u.GameplayStatics.get_all_actors_of_class(world, sample_class):
    mesh = sample.get_components_by_class(u.SkeletalMeshComponent)[0]
    location = sample.get_actor_location()
    material = mesh.get_material(0)
    rows.append({
        'label': sample.get_actor_label(),
        'path': sample.get_path_name(),
        'location': [location.x, location.y, location.z],
        'hidden': sample.get_editor_property('hidden'),
        'component_visible': mesh.is_visible(),
        'component_hidden_in_game': mesh.get_editor_property('hidden_in_game'),
        'overlay_material': (
            mesh.get_editor_property('overlay_material').get_path_name()
            if mesh.get_editor_property('overlay_material') else None
        ),
        'material': material.get_path_name() if material else None,
        'parent': (
            material.parent.get_path_name()
            if isinstance(material, u.MaterialInstanceDynamic) and material.parent
            else None
        ),
    })
out = u.Paths.project_dir() + 'Saved/RadiantDissolve/pie_samples.json'
with open(out, 'w', encoding='utf-8') as handle:
    json.dump(rows, handle, ensure_ascii=False, indent=2)
print('RADIANT_PIE_SAMPLES_INSPECTED')
