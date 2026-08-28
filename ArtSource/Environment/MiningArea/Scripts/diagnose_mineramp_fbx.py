import bpy
import json
import math
from collections import Counter
from mathutils import Vector


FBX_PATH = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\MiningArea\Export\SM_MineRamp_Main.fbx"

before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=FBX_PATH, use_custom_normals=True)
imported = [obj for obj in bpy.data.objects if obj not in before and obj.type == "MESH"]
if len(imported) != 1:
    raise RuntimeError(f"Expected one imported mesh, found {len(imported)}")

obj = imported[0]
mesh = obj.data
mesh.calc_loop_triangles()
uv_layer = mesh.uv_layers.active
if uv_layer is None:
    raise RuntimeError("Imported FBX has no active UV layer")

zero_geom = []
zero_uv = []
min_geom = math.inf
min_uv = math.inf
zero_uv_materials = Counter()
for index, triangle in enumerate(mesh.loop_triangles):
    points = [mesh.vertices[vertex_index].co for vertex_index in triangle.vertices]
    geom_area = ((points[1] - points[0]).cross(points[2] - points[0])).length * 0.5
    uvs = [uv_layer.data[loop_index].uv for loop_index in triangle.loops]
    uv_area = abs((uvs[1] - uvs[0]).cross(uvs[2] - uvs[0])) * 0.5
    min_geom = min(min_geom, geom_area)
    min_uv = min(min_uv, uv_area)
    if geom_area <= 1e-12:
        zero_geom.append(index)
    if uv_area <= 1e-12:
        zero_uv.append(index)
        material_name = (
            mesh.materials[triangle.material_index].name
            if triangle.material_index < len(mesh.materials) and mesh.materials[triangle.material_index]
            else "<None>"
        )
        zero_uv_materials[material_name] += 1

tangent_error = None
near_zero_tangent_loops = None
near_zero_tangent_details = []
try:
    mesh.calc_tangents(uvmap=uv_layer.name)
    for polygon in mesh.polygons:
        material_name = (
            mesh.materials[polygon.material_index].name
            if polygon.material_index < len(mesh.materials) and mesh.materials[polygon.material_index]
            else "<None>"
        )
        for loop_index in polygon.loop_indices:
            tangent_length = Vector(mesh.loops[loop_index].tangent).length
            if tangent_length <= 1e-4:
                near_zero_tangent_details.append(
                    {
                        "polygon": polygon.index,
                        "loop": loop_index,
                        "material": material_name,
                        "polygon_area": polygon.area,
                        "polygon_normal": list(polygon.normal),
                        "vertices": [list(mesh.vertices[vertex_index].co) for vertex_index in polygon.vertices],
                        "uvs": [list(uv_layer.data[index].uv) for index in polygon.loop_indices],
                        "loop_normal": list(mesh.loops[loop_index].normal),
                        "tangent_length": tangent_length,
                    }
                )
    near_zero_tangent_loops = len(near_zero_tangent_details)
except Exception as exc:
    tangent_error = repr(exc)

result = {
    "fbx": FBX_PATH,
    "object": obj.name,
    "vertices": len(mesh.vertices),
    "polygons": len(mesh.polygons),
    "triangles": len(mesh.loop_triangles),
    "uv_layer": uv_layer.name,
    "zero_geometry_triangles": len(zero_geom),
    "zero_uv_triangles": len(zero_uv),
    "zero_uv_materials": dict(zero_uv_materials),
    "min_geometry_area": min_geom,
    "min_uv_area": min_uv,
    "near_zero_tangent_loops": near_zero_tangent_loops,
    "near_zero_tangent_details": near_zero_tangent_details[:20],
    "tangent_error": tangent_error,
    "first_zero_geometry_triangles": zero_geom[:20],
    "first_zero_uv_triangles": zero_uv[:20],
}
print("MINERAMP_DIAG=" + json.dumps(result, ensure_ascii=False, separators=(",", ":")))
