import json
import unreal as u

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
OUT = u.Paths.project_dir() + 'Tools/RadiantDissolve/inspect_v2.json'


def asset_info(path):
    asset = u.load_asset(path)
    if not asset:
        return {'path': path, 'missing': True}
    result = {'path': path, 'class': asset.get_class().get_name()}
    if isinstance(asset, u.StaticMesh):
        result['allow_cpu_access'] = asset.get_editor_property('allow_cpu_access')
        result['materials'] = [
            slot.get_editor_property('material_interface').get_path_name()
            if slot.get_editor_property('material_interface') else None
            for slot in asset.get_editor_property('static_materials')
        ]
        bounds = asset.get_bounds()
        result['bounds_extent'] = [bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z]
    if isinstance(asset, u.SkeletalMesh):
        result['materials'] = [
            slot.get_editor_property('material_interface').get_path_name()
            if slot.get_editor_property('material_interface') else None
            for slot in asset.get_editor_property('materials')
        ]
        bounds = asset.get_bounds()
        result['bounds_extent'] = [bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z]
    return result


result = {
    'assets': [
        asset_info('/Game/MineLearning/Characters/Gunner/Weapons/AK/Meshes/SM_AK'),
        asset_info('/Game/MineLearning/Characters/Gunner/Meshes/SK_Gunner'),
        asset_info(ROOT + '/M_RadiantDissolve_Test'),
        asset_info(ROOT + '/M_RadiantDissolve_Surface'),
    ],
    'actors': [],
    'material_custom_nodes': {},
}

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    components = []
    for component in actor.get_components_by_class(u.MeshComponent):
        item = {'name': component.get_name(), 'class': component.get_class().get_name()}
        if isinstance(component, u.StaticMeshComponent) and component.static_mesh:
            item['mesh'] = component.static_mesh.get_path_name()
        elif isinstance(component, u.SkeletalMeshComponent) and component.skeletal_mesh:
            item['mesh'] = component.skeletal_mesh.get_path_name()
        item['materials'] = [
            component.get_material(i).get_path_name() if component.get_material(i) else None
            for i in range(component.get_num_materials())
        ]
        components.append(item)
    result['actors'].append({
        'label': actor.get_actor_label(),
        'class': actor.get_class().get_path_name(),
        'components': components,
    })

for material_name in ['M_RadiantDissolve_Test', 'M_RadiantDissolve_Surface', 'M_RadiantMote']:
    material = u.load_asset(ROOT + '/' + material_name)
    nodes = []
    if material:
        for expression in u.MaterialEditingLibrary.get_material_expressions(material):
            if isinstance(expression, u.MaterialExpressionCustom):
                nodes.append({
                    'description': expression.get_editor_property('description'),
                    'code': expression.get_editor_property('code'),
                })
        result['material_custom_nodes'][material_name] = nodes

with open(OUT, 'w', encoding='utf-8') as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
print('RADIANT_V2_INSPECTED')
