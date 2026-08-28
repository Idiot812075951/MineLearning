import bpy
import bmesh
import json
from mathutils import Vector


def world_bounds(obj):
    corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    return {
        "min": [round(min(v[i] for v in corners), 5) for i in range(3)],
        "max": [round(max(v[i] for v in corners), 5) for i in range(3)],
    }


def topology(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    result = {
        "nonmanifold_edges": sum(1 for edge in bm.edges if not edge.is_manifold),
        "loose_edges": sum(1 for edge in bm.edges if not edge.link_faces),
    }
    bm.free()
    return result


scene = bpy.context.scene
objects = []
for obj in sorted(bpy.data.objects, key=lambda item: item.name):
    entry = {
        "name": obj.name,
        "type": obj.type,
        "collections": sorted(collection.name for collection in obj.users_collection),
        "location": [round(value, 6) for value in obj.location],
        "rotation": [round(value, 6) for value in obj.rotation_euler],
        "scale": [round(value, 6) for value in obj.scale],
        "parent": obj.parent.name if obj.parent else None,
        "hidden_viewport": obj.hide_viewport,
        "hidden_render": obj.hide_render,
        "export_candidate": obj.get("RC_ExportCandidate"),
    }
    if obj.type == "MESH":
        entry.update({
            "bounds": world_bounds(obj),
            "dimensions": [round(value, 5) for value in obj.dimensions],
            "vertices": len(obj.data.vertices),
            "polygons": len(obj.data.polygons),
            "materials": [slot.material.name if slot.material else None for slot in obj.material_slots],
            "uv_layers": [layer.name for layer in obj.data.uv_layers],
            "modifiers": [[modifier.name, modifier.type, modifier.show_render] for modifier in obj.modifiers],
            **topology(obj.data),
        })
    objects.append(entry)

root_collections = [collection.name for collection in scene.collection.children]
payload = {
    "filepath": bpy.data.filepath,
    "is_saved": bpy.data.is_saved,
    "is_dirty": bpy.data.is_dirty,
    "scene": scene.name,
    "unit_system": scene.unit_settings.system,
    "unit_scale": scene.unit_settings.scale_length,
    "fps": scene.render.fps,
    "root_collections": root_collections,
    "object_count": len(bpy.data.objects),
    "mesh_count": len(bpy.data.meshes),
    "objects": objects,
}
modules = []
for collection in sorted(
    (item for item in bpy.data.collections if item.name.startswith("ASSET_")),
    key=lambda item: item.name,
):
    members = list(collection.all_objects)
    meshes = [obj for obj in members if obj.type == "MESH"]
    roots = [obj for obj in members if obj.type == "EMPTY" and obj.name.startswith("ROOT_")]
    if not meshes:
        continue
    corners = [obj.matrix_world @ Vector(corner) for obj in meshes for corner in obj.bound_box]
    modules.append({
        "collection": collection.name,
        "asset_name": collection.name.removeprefix("ASSET_"),
        "root_objects": [obj.name for obj in roots],
        "root_transforms": {
            obj.name: {
                "location": [round(value, 6) for value in obj.location],
                "rotation": [round(value, 6) for value in obj.rotation_euler],
                "scale": [round(value, 6) for value in obj.scale],
            }
            for obj in roots
        },
        "mesh_count": len(meshes),
        "mesh_names": sorted(obj.name for obj in meshes),
        "bounds": {
            "min": [round(min(v[i] for v in corners), 5) for i in range(3)],
            "max": [round(max(v[i] for v in corners), 5) for i in range(3)],
        },
        "vertices": sum(len(obj.data.vertices) for obj in meshes),
        "polygons": sum(len(obj.data.polygons) for obj in meshes),
        "materials": sorted({slot.material.name for obj in meshes for slot in obj.material_slots if slot.material}),
        "all_have_uv": all(bool(obj.data.uv_layers) for obj in meshes),
        "bad_scale": [obj.name for obj in meshes if any(abs(value - 1.0) > 1e-6 for value in obj.scale)],
        "bad_rotation": [obj.name for obj in meshes if any(abs(value) > 1e-6 for value in obj.rotation_euler)],
        "nonmanifold_edges": sum(topology(obj.data)["nonmanifold_edges"] for obj in meshes),
        "loose_edges": sum(topology(obj.data)["loose_edges"] for obj in meshes),
        "modifier_types": sorted({modifier.type for obj in meshes for modifier in obj.modifiers if modifier.show_render}),
    })

compact = {
    "filepath": payload["filepath"],
    "is_saved": payload["is_saved"],
    "is_dirty": payload["is_dirty"],
    "scene": payload["scene"],
    "unit_system": payload["unit_system"],
    "unit_scale": payload["unit_scale"],
    "object_count": payload["object_count"],
    "mesh_count": payload["mesh_count"],
    "module_count": len(modules),
    "modules": modules,
    "non_asset_meshes": sorted(
        obj.name for obj in bpy.data.objects
        if obj.type == "MESH" and not any(collection.name.startswith("ASSET_") for collection in obj.users_collection)
    ),
}
print("MININGAREA_MODULES=" + json.dumps(compact, ensure_ascii=False, separators=(",", ":")))
