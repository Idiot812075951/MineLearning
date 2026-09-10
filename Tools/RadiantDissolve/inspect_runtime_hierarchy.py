"""Dump the active dissolve target, attached actors, mesh components and materials."""
import json
from pathlib import Path

import unreal as u


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
controller_class = u.load_class(
    None,
    ROOT + '/BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C',
)
controllers = u.GameplayStatics.get_all_actors_of_class(world, controller_class)
if not controllers:
    raise RuntimeError('No BP_RadiantDissolve_Test in the PIE world')

controller = controllers[0]
target = controller.get_editor_property('RadiantTarget')
if not target:
    raise RuntimeError('RadiantTarget is empty')


def material_record(material):
    if not material:
        return {'material': None, 'base': None}
    base = material.get_base_material()
    return {
        'material': material.get_path_name(),
        'class': material.get_class().get_name(),
        'base': base.get_path_name() if base else None,
    }


visited = set()
rows = []


def visit(actor, relation):
    if not actor or actor.get_path_name() in visited:
        return
    visited.add(actor.get_path_name())
    parent_component = actor.get_attach_parent_actor()
    actor_row = {
        'actor': actor.get_path_name(),
        'label': actor.get_actor_label(),
        'class': actor.get_class().get_path_name(),
        'relation': relation,
        'attach_parent_actor': (
            parent_component.get_path_name() if parent_component else None
        ),
        'meshes': [],
    }
    mesh_classes = (u.StaticMeshComponent, u.SkeletalMeshComponent)
    for component in actor.get_components_by_class(u.MeshComponent):
        if not isinstance(component, mesh_classes):
            continue
        asset = None
        if isinstance(component, u.StaticMeshComponent):
            asset = component.get_editor_property('static_mesh')
        else:
            asset = component.get_editor_property('skeletal_mesh_asset')
        actor_row['meshes'].append({
            'component': component.get_path_name(),
            'name': component.get_name(),
            'type': component.get_class().get_name(),
            'asset': asset.get_path_name() if asset else None,
            'visible': component.is_visible(),
            'materials': [
                material_record(component.get_material(slot))
                for slot in range(component.get_num_materials())
            ],
        })
    rows.append(actor_row)

    children = actor.get_attached_actors(True, True)
    for child in children:
        visit(child, 'attached')
    for child_component in actor.get_components_by_class(u.ChildActorComponent):
        child = child_component.get_child_actor()
        if child:
            visit(child, 'child_actor_component')


visit(target, 'target')
payload = {
    'controller': controller.get_path_name(),
    'target': target.get_path_name(),
    'collected_static': [
        component.get_path_name()
        for component in controller.get_editor_property('RadiantStaticMeshes')
    ],
    'collected_skeletal': [
        component.get_path_name()
        for component in controller.get_editor_property('RadiantSkeletalMeshes')
    ],
    'hierarchy': rows,
}
output = Path(u.Paths.project_saved_dir()) / 'RadiantDissolve' / 'target_hierarchy.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(payload, indent=2), encoding='utf-8')
print('RADIANT_HIERARCHY_DUMPED', output)
