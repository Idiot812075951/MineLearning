"""Write relevant test-level actor labels, classes and transforms."""
import json
import unreal as u

out = u.Paths.project_dir() + 'Saved/RadiantDissolve/test_level_actors.json'
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
rows = []
for actor in actors:
    label = actor.get_actor_label()
    if 'Radiant' not in label and 'Dissolve' not in label:
        continue
    location = actor.get_actor_location()
    rows.append({
        'label': label,
        'class': actor.get_class().get_name(),
        'path': actor.get_path_name(),
        'location': [location.x, location.y, location.z],
    })
with open(out, 'w', encoding='utf-8') as handle:
    json.dump(rows, handle, ensure_ascii=False, indent=2)
print('RADIANT_TEST_LEVEL_INSPECTED')
