"""Write a compact diagnostic report for the Gunner dissolve adapter."""
import json
import unreal as u

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
OUT = u.Paths.project_dir() + 'Saved/RadiantDissolve/gunner_adapter.json'
material = u.load_asset(ROOT + '/M_RadiantDissolve_Gunner')
editing = u.MaterialEditingLibrary


def input_for(prop):
    node = editing.get_material_property_input_node(material, prop)
    return {
        'node': node.get_name() if node else None,
        'class': node.get_class().get_name() if node else None,
        'output': editing.get_material_property_input_node_output_name(material, prop) if node else None,
    }


report = {
    'blend_mode': str(material.get_editor_property('blend_mode')),
    'shading_model': str(material.get_editor_property('shading_model')),
    'use_material_attributes': material.get_editor_property('use_material_attributes'),
    'opacity_mask_clip_value': material.get_editor_property('opacity_mask_clip_value'),
    'used_with_skeletal_mesh': material.get_editor_property('used_with_skeletal_mesh'),
    'inputs': {
        'base_color': input_for(u.MaterialProperty.MP_BASE_COLOR),
        'emissive': input_for(u.MaterialProperty.MP_EMISSIVE_COLOR),
        'opacity_mask': input_for(u.MaterialProperty.MP_OPACITY_MASK),
        'roughness': input_for(u.MaterialProperty.MP_ROUGHNESS),
        'normal': input_for(u.MaterialProperty.MP_NORMAL),
    },
    'custom_nodes': [
        {
            'name': node.get_name(),
            'description': node.get_editor_property('description'),
            'code': node.get_editor_property('code'),
        }
        for node in editing.get_material_expressions(material)
        if isinstance(node, u.MaterialExpressionCustom)
    ],
}
with open(OUT, 'w', encoding='utf-8') as handle:
    json.dump(report, handle, ensure_ascii=False, indent=2)
print('RADIANT_GUNNER_ADAPTER_INSPECTED')
