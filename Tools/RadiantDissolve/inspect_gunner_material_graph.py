"""Write a compact report of M_Gunner's root graph shape."""
import json
from pathlib import Path

import unreal as u


material = u.load_asset('/Game/MineLearning/Characters/Gunner/Materials/M_Gunner')
editing = u.MaterialEditingLibrary
properties = {}
for prop in (
    u.MaterialProperty.MP_MATERIAL_ATTRIBUTES,
    u.MaterialProperty.MP_BASE_COLOR,
    u.MaterialProperty.MP_EMISSIVE_COLOR,
    u.MaterialProperty.MP_OPACITY,
    u.MaterialProperty.MP_OPACITY_MASK,
    u.MaterialProperty.MP_NORMAL,
    u.MaterialProperty.MP_ROUGHNESS,
):
    node = editing.get_material_property_input_node(material, prop)
    properties[str(prop)] = {
        'node': node.get_class().get_name() if node else None,
        'name': node.get_name() if node else None,
        'output': editing.get_material_property_input_node_output_name(material, prop) if node else None,
    }
payload = {
    'use_material_attributes': material.get_editor_property('use_material_attributes'),
    'blend_mode': str(material.get_editor_property('blend_mode')),
    'properties': properties,
}
output = Path(u.Paths.project_saved_dir()) / 'RadiantDissolve' / 'gunner_graph.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(payload, indent=2), encoding='utf-8')
print('RADIANT_GUNNER_GRAPH_INSPECTED')
