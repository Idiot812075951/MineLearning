import bpy
import bmesh
import hashlib
import json
import os
from mathutils import Matrix


EXPECTED_BLEND = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\MiningArea\MiningArea_P2_Environment_Source.blend"
EXPORT_DIR = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\MiningArea\Export"


def export_fbx(obj, filepath):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=filepath,
        use_selection=True,
        object_types={"MESH"},
        global_scale=100.0,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        use_space_transform=True,
        bake_space_transform=False,
        axis_forward="-X",
        axis_up="Z",
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        use_tspace=True,
        use_triangles=False,
        use_custom_props=True,
        add_leaf_bones=False,
        bake_anim=False,
        path_mode="AUTO",
        embed_textures=False,
    )


def bounds_from_mesh(mesh):
    return {
        "min": [round(min(vertex.co[i] for vertex in mesh.vertices), 6) for i in range(3)],
        "max": [round(max(vertex.co[i] for vertex in mesh.vertices), 6) for i in range(3)],
    }


def remove_empty_material_slots(mesh):
    old_materials = list(mesh.materials)
    assigned = [
        old_materials[polygon.material_index]
        if polygon.material_index < len(old_materials) else None
        for polygon in mesh.polygons
    ]
    if any(material is None for material in assigned):
        raise RuntimeError("Export mesh contains faces assigned to an empty material slot")
    ordered = []
    for material in old_materials:
        if material is not None and material not in ordered:
            ordered.append(material)
    mesh.materials.clear()
    for material in ordered:
        mesh.materials.append(material)
    material_indices = {material: index for index, material in enumerate(ordered)}
    for polygon, material in zip(mesh.polygons, assigned):
        polygon.material_index = material_indices[material]


def rebuild_export_uv(obj, method="SMART"):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    while obj.data.uv_layers:
        obj.data.uv_layers.remove(obj.data.uv_layers[0])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    if method == "CUBE":
        bpy.ops.uv.cube_project(
            cube_size=1.0,
            correct_aspect=True,
            clip_to_bounds=False,
            scale_to_bounds=False,
        )
    else:
        bpy.ops.uv.smart_project(
            angle_limit=1.151917,
            island_margin=0.02,
            area_weight=0.0,
            correct_aspect=True,
            scale_to_bounds=False,
        )
    bpy.ops.object.mode_set(mode="OBJECT")


def triangulate_export_mesh(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(
        bm,
        faces=list(bm.faces),
        quad_method="BEAUTY",
        ngon_method="BEAUTY",
    )
    # Bevel-generated sliver triangles on the main ramp can survive as valid
    # geometry while still falling below Unreal's tangent tolerance.  This
    # threshold remains below the smallest intentional ramp surface.
    degenerate_faces = [face for face in bm.faces if face.calc_area() <= 1e-5]
    if degenerate_faces:
        bmesh.ops.delete(bm, geom=degenerate_faces, context="FACES")
    bm.to_mesh(mesh)
    bm.free()
    mesh.update(calc_edges=True)


def invalid_tangent_polygon_indices(mesh):
    uv_layer = mesh.uv_layers.active
    if uv_layer is None:
        raise RuntimeError("Cannot validate tangents without an active UV layer")
    mesh.calc_tangents(uvmap=uv_layer.name)
    invalid_polygon_indices = {
        polygon.index
        for polygon in mesh.polygons
        if any(mesh.loops[loop_index].tangent.length <= 1e-4 for loop_index in polygon.loop_indices)
    }
    mesh.free_tangents()
    return invalid_polygon_indices


if os.path.normcase(os.path.abspath(bpy.data.filepath)) != os.path.normcase(os.path.abspath(EXPECTED_BLEND)):
    raise RuntimeError(f"Wrong MiningArea source: {bpy.data.filepath!r}")
if not bpy.data.is_saved:
    raise RuntimeError("MiningArea source must be saved before export")
source_dirty_on_background_load = bpy.data.is_dirty

os.makedirs(EXPORT_DIR, exist_ok=True)
depsgraph = bpy.context.evaluated_depsgraph_get()
asset_collections = sorted(
    (collection for collection in bpy.data.collections if collection.name.startswith("ASSET_SM_")),
    key=lambda collection: collection.name,
)
if not asset_collections:
    raise RuntimeError("No ASSET_SM_* collections found")

temporary_collection = bpy.data.collections.new("UE_EXPORT_TEMP")
bpy.context.scene.collection.children.link(temporary_collection)
report = []

for collection in asset_collections:
    asset_name = collection.name.removeprefix("ASSET_")
    members = list(collection.all_objects)
    sources = sorted((obj for obj in members if obj.type == "MESH"), key=lambda obj: obj.name)
    roots = [obj for obj in members if obj.type == "EMPTY" and obj.name == f"ROOT_{asset_name}"]
    if not sources:
        continue
    if len(roots) != 1:
        raise RuntimeError(f"{asset_name}: expected one ROOT_{asset_name}, found {len(roots)}")
    root = roots[0]
    if any(abs(value) > 1e-6 for value in root.rotation_euler) or any(abs(value - 1.0) > 1e-6 for value in root.scale):
        raise RuntimeError(f"{asset_name}: root transform is not export-safe")

    carrier_mesh = bpy.data.meshes.new(f"{asset_name}_UEExportMesh")
    carrier = bpy.data.objects.new(asset_name, carrier_mesh)
    temporary_collection.objects.link(carrier)
    carrier.matrix_world = Matrix.Identity(4)
    carrier["ML_SourceBlend"] = EXPECTED_BLEND
    carrier["ML_SourceCollection"] = collection.name
    carrier["ML_SourceRoot"] = root.name

    temporary_parts = []
    for source in sources:
        evaluated = source.evaluated_get(depsgraph)
        mesh = bpy.data.meshes.new_from_object(
            evaluated,
            preserve_all_data_layers=True,
            depsgraph=depsgraph,
        )
        part = bpy.data.objects.new(f"TMP_{source.name}", mesh)
        temporary_collection.objects.link(part)
        part.matrix_world = root.matrix_world.inverted() @ source.matrix_world
        temporary_parts.append(part)

    bpy.ops.object.select_all(action="DESELECT")
    carrier.select_set(True)
    for part in temporary_parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = carrier
    bpy.ops.object.join()
    carrier.data.name = f"{asset_name}_UEExportMesh"
    remove_empty_material_slots(carrier.data)
    if asset_name == "SM_MineRamp_Main":
        triangulate_export_mesh(carrier.data)
    if asset_name == "SM_MineRamp_Main":
        rebuild_export_uv(carrier, method="CUBE")
        invalid_polygons = invalid_tangent_polygon_indices(carrier.data)
        carrier["ML_ExportKnownNearZeroTangentPolygons"] = len(invalid_polygons)
    else:
        rebuild_export_uv(carrier, method="SMART")
    carrier.data.validate(clean_customdata=False)
    carrier.data.update(calc_edges=True)

    filepath = os.path.join(EXPORT_DIR, f"{asset_name}.fbx")
    export_fbx(carrier, filepath)
    with open(filepath, "rb") as handle:
        digest = hashlib.sha256(handle.read()).hexdigest()
    report.append({
        "asset": asset_name,
        "source_collection": collection.name,
        "source_root": root.name,
        "source_part_count": len(sources),
        "bounds_m": bounds_from_mesh(carrier.data),
        "vertices": len(carrier.data.vertices),
        "polygons": len(carrier.data.polygons),
        "uv_layers": [layer.name for layer in carrier.data.uv_layers],
        "materials": [material.name if material else None for material in carrier.data.materials],
        "fbx": filepath,
        "bytes": os.path.getsize(filepath),
        "sha256": digest,
    })
    mesh = carrier.data
    bpy.data.objects.remove(carrier, do_unlink=True)
    if mesh.users == 0:
        bpy.data.meshes.remove(mesh)

bpy.data.collections.remove(temporary_collection)
EXPORT_RESULT = {
    "source": bpy.data.filepath,
    "source_dirty_on_background_load": source_dirty_on_background_load,
    "export_dir": EXPORT_DIR,
    "asset_count": len(report),
    "assets": report,
}
print("MININGAREA_EXPORT=" + json.dumps(EXPORT_RESULT, ensure_ascii=False, separators=(",", ":")))
