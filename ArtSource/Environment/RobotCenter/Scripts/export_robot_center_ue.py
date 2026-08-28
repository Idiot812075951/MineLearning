import bpy
import hashlib
import json
import os
from mathutils import Matrix, Vector


EXPECTED_BLEND = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\RobotCenter\RobotCenter.blend"
EXPORT_DIR = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\RobotCenter\Export"


def bounds_from_mesh(mesh):
    if not mesh.vertices:
        return {"min": [0.0, 0.0, 0.0], "max": [0.0, 0.0, 0.0]}
    return {
        "min": [round(min(vertex.co[i] for vertex in mesh.vertices), 6) for i in range(3)],
        "max": [round(max(vertex.co[i] for vertex in mesh.vertices), 6) for i in range(3)],
    }


def rebuild_export_uv(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    while obj.data.uv_layers:
        obj.data.uv_layers.remove(obj.data.uv_layers[0])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(
        angle_limit=1.151917,
        island_margin=0.02,
        area_weight=0.0,
        correct_aspect=True,
        scale_to_bounds=False,
    )
    bpy.ops.object.mode_set(mode="OBJECT")


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


if os.path.normcase(os.path.abspath(bpy.data.filepath)) != os.path.normcase(os.path.abspath(EXPECTED_BLEND)):
    raise RuntimeError(f"Wrong Robot Center source: {bpy.data.filepath!r}")
if not bpy.data.is_saved:
    raise RuntimeError("Robot Center source must be saved before export")
source_dirty_on_background_load = bpy.data.is_dirty

os.makedirs(EXPORT_DIR, exist_ok=True)
depsgraph = bpy.context.evaluated_depsgraph_get()
candidates = sorted(
    (
        obj for obj in bpy.data.objects
        if obj.get("RC_ExportCandidate") is True and obj.type in {"MESH", "CURVE"}
    ),
    key=lambda obj: obj.name,
)
if not candidates:
    raise RuntimeError("No RC_ExportCandidate objects found")

report = []
for source in candidates:
    evaluated = source.evaluated_get(depsgraph)
    mesh = bpy.data.meshes.new_from_object(
        evaluated,
        preserve_all_data_layers=True,
        depsgraph=depsgraph,
    )
    mesh.name = f"{source.name}_UEExportMesh"
    mesh.validate(clean_customdata=False)
    mesh.update(calc_edges=True)
    export_obj = bpy.data.objects.new(source.name, mesh)
    bpy.context.scene.collection.objects.link(export_obj)
    export_obj.matrix_world = Matrix.Identity(4)
    export_obj["ML_SourceBlend"] = EXPECTED_BLEND
    export_obj["ML_SourceObject"] = source.name
    export_obj["ML_SourceOriginMeters"] = [round(value, 6) for value in source.location]
    rebuild_export_uv(export_obj)

    filepath = os.path.join(EXPORT_DIR, f"{source.name}.fbx")
    export_fbx(export_obj, filepath)
    with open(filepath, "rb") as handle:
        digest = hashlib.sha256(handle.read()).hexdigest()
    report.append({
        "asset": source.name,
        "source_type": source.type,
        "source_origin_m": [round(value, 6) for value in source.location],
        "bounds_m": bounds_from_mesh(mesh),
        "vertices": len(mesh.vertices),
        "polygons": len(mesh.polygons),
        "uv_layers": [layer.name for layer in mesh.uv_layers],
        "materials": [material.name if material else None for material in mesh.materials],
        "fbx": filepath,
        "bytes": os.path.getsize(filepath),
        "sha256": digest,
    })
    bpy.data.objects.remove(export_obj, do_unlink=True)
    if mesh.users == 0:
        bpy.data.meshes.remove(mesh)

EXPORT_RESULT = {
    "source": bpy.data.filepath,
    "source_dirty_on_background_load": source_dirty_on_background_load,
    "export_dir": EXPORT_DIR,
    "asset_count": len(report),
    "assets": report,
}
print("ROBOTCENTER_EXPORT=" + json.dumps(EXPORT_RESULT, ensure_ascii=False, separators=(",", ":")))
