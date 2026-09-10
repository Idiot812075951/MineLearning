"""Record skeletal sample materials in the editor world."""
import json
import unreal as u

rows = []
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() != 'BP_RadiantSkeletalTargetSample_C':
        continue
    meshes = actor.get_components_by_class(u.SkeletalMeshComponent)
    rows.append({
        'label': actor.get_actor_label(),
        'materials': [
            mesh.get_material(index).get_path_name() if mesh.get_material(index) else None
            for mesh in meshes
            for index in range(mesh.get_num_materials())
        ],
    })

out = u.Paths.project_saved_dir() + 'RadiantDissolve/editor_samples.json'
with open(out, 'w', encoding='utf-8') as handle:
    json.dump(rows, handle, ensure_ascii=False, indent=2)
print('RADIANT_EDITOR_SAMPLES_INSPECTED')
