import bpy
import json


collection = bpy.data.collections["ASSET_SM_MineRamp_Main"]
depsgraph = bpy.context.evaluated_depsgraph_get()
result = []
for source in sorted((obj for obj in collection.all_objects if obj.type == "MESH"), key=lambda obj: obj.name):
    evaluated = source.evaluated_get(depsgraph)
    mesh = bpy.data.meshes.new_from_object(evaluated, preserve_all_data_layers=True, depsgraph=depsgraph)
    mesh.calc_loop_triangles()
    zero_triangles = []
    positive_areas = []
    minimum_area = None
    for index, triangle in enumerate(mesh.loop_triangles):
        points = [mesh.vertices[vertex_index].co for vertex_index in triangle.vertices]
        area = ((points[1] - points[0]).cross(points[2] - points[0])).length * 0.5
        minimum_area = area if minimum_area is None else min(minimum_area, area)
        if area <= 1e-12:
            zero_triangles.append(index)
        else:
            positive_areas.append(area)
    result.append({
        "object": source.name,
        "dimensions": [round(value, 8) for value in source.dimensions],
        "materials": [slot.material.name if slot.material else None for slot in source.material_slots],
        "vertices": len(mesh.vertices),
        "triangles": len(mesh.loop_triangles),
        "zero_triangles": len(zero_triangles),
        "min_area": minimum_area,
        "smallest_positive_areas": sorted(positive_areas)[:12],
        "modifiers": [[modifier.name, modifier.type] for modifier in source.modifiers if modifier.show_render],
    })
    bpy.data.meshes.remove(mesh)

print("MINERAMP_SOURCE_DIAG=" + json.dumps(result, ensure_ascii=False, separators=(",", ":")))
