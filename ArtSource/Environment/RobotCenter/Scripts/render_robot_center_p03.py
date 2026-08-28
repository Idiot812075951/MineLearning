import bpy
import os
from mathutils import Vector


TARGET_BLEND = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\RobotCenter\RobotCenter.blend"
PREVIEW_DIR = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\RobotCenter\Previews"


def point_camera(camera, location, target):
    camera.location = location
    direction = Vector(target) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def configure_scene(scene):
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1200
    scene.render.resolution_y = 800
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = False
    scene.render.use_file_extension = True
    scene.render.image_settings.color_depth = "8"
    try:
        scene.view_settings.look = "AgX - Medium High Contrast"
    except TypeError:
        pass
    world = scene.world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    if background:
        background.inputs["Color"].default_value = (0.008, 0.012, 0.020, 1.0)
        background.inputs["Strength"].default_value = 0.20


def set_scale_proxies_hidden(hidden):
    for name in ("REF_Human_1p8m", "REF_OreBuddy_1p1m"):
        obj = bpy.data.objects.get(name)
        if obj:
            obj.hide_render = hidden


def set_review_paths_hidden(hidden):
    for name in ("REVIEW_Path_RobotExit", "REVIEW_Path_TransformApproach"):
        obj = bpy.data.objects.get(name)
        if obj:
            obj.hide_render = hidden


def show_single_proxy(name, location):
    set_scale_proxies_hidden(True)
    obj = bpy.data.objects.get(name)
    if obj is None:
        raise RuntimeError(f"Scale proxy missing: {name}")
    obj.location = location
    obj.hide_render = False


def render_view(scene, camera, filename, camera_type, location, target=None, lens=52.0, ortho_scale=10.0):
    camera.data.type = camera_type
    if camera_type == "ORTHO":
        camera.data.ortho_scale = ortho_scale
    else:
        camera.data.lens = lens
    if target is None:
        camera.location = location
        camera.rotation_euler = (0.0, 0.0, 0.0)
    else:
        point_camera(camera, location, target)
    path = os.path.join(PREVIEW_DIR, filename)
    scene.render.filepath = path
    bpy.context.view_layer.update()
    bpy.ops.render.render(write_still=True)
    return {
        "path": path,
        "exists": os.path.exists(path),
        "size_bytes": os.path.getsize(path) if os.path.exists(path) else None,
    }


def render_all():
    if os.path.normcase(os.path.abspath(bpy.data.filepath)) != os.path.normcase(os.path.abspath(TARGET_BLEND)):
        raise RuntimeError(f"Wrong Blender file connected: {bpy.data.filepath!r}")
    scene = bpy.context.scene
    camera = bpy.data.objects.get("PREVIEW_Camera")
    if camera is None or camera.type != "CAMERA":
        raise RuntimeError("PREVIEW_Camera is missing")
    os.makedirs(PREVIEW_DIR, exist_ok=True)
    configure_scene(scene)
    scene.camera = camera

    proxy_matrices = {
        name: bpy.data.objects[name].matrix_world.copy()
        for name in ("REF_Human_1p8m", "REF_OreBuddy_1p1m")
    }
    renders = []
    set_scale_proxies_hidden(True)
    set_review_paths_hidden(True)
    renders.append(render_view(
        scene, camera, "RobotCenter_P03_Review_Front.png", "ORTHO",
        (-1.20, -22.0, 3.10), target=(-1.20, 0.1, 2.55), ortho_scale=13.8,
    ))
    renders.append(render_view(
        scene, camera, "RobotCenter_P03_Review_Top.png", "ORTHO",
        (-1.20, -1.0, 20.0), target=None, ortho_scale=13.4,
    ))
    renders.append(render_view(
        scene, camera, "RobotCenter_P03_Review_45deg.png", "PERSP",
        (15.0, -25.0, 14.5), target=(-1.15, -0.25, 2.20), lens=50.0,
    ))
    show_single_proxy("REF_Human_1p8m", (3.32, -1.12, 1.34))
    renders.append(render_view(
        scene, camera, "RobotCenter_P03_Review_HumanScale.png", "PERSP",
        (10.4, -12.2, 6.9), target=(3.28, -0.68, 2.02), lens=58.0,
    ))
    show_single_proxy("REF_OreBuddy_1p1m", (3.32, -1.12, 1.34))
    renders.append(render_view(
        scene, camera, "RobotCenter_P03_Review_OreBuddyScale.png", "PERSP",
        (10.4, -12.2, 6.9), target=(3.28, -0.68, 1.72), lens=58.0,
    ))
    set_scale_proxies_hidden(True)
    set_review_paths_hidden(False)
    renders.append(render_view(
        scene, camera, "RobotCenter_P03_Review_TopPath.png", "ORTHO",
        (-1.20, -1.0, 20.0), target=None, ortho_scale=13.4,
    ))

    # Leave the source file opening on the most useful review angle.
    for name, matrix in proxy_matrices.items():
        bpy.data.objects[name].matrix_world = matrix
    set_scale_proxies_hidden(True)
    set_review_paths_hidden(True)
    camera.data.type = "PERSP"
    camera.data.lens = 50.0
    point_camera(camera, (15.0, -25.0, 14.5), (-1.15, -0.25, 2.20))
    scene.render.filepath = renders[2]["path"]
    bpy.ops.wm.save_as_mainfile(filepath=TARGET_BLEND, check_existing=False)
    return {"renders": renders, "camera": camera.name, "filepath": bpy.data.filepath}


RENDER_RESULT = render_all()
