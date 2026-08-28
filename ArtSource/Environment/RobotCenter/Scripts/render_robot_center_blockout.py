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

    renders = []
    renders.append(render_view(
        scene, camera, "RobotCenter_Blockout_Front.png", "ORTHO",
        (0.0, -20.0, 3.05), target=(0.0, 0.2, 2.45), ortho_scale=15.5,
    ))
    renders.append(render_view(
        scene, camera, "RobotCenter_Blockout_Top.png", "ORTHO",
        (0.0, -0.75, 20.0), target=None, ortho_scale=16.5,
    ))
    renders.append(render_view(
        scene, camera, "RobotCenter_Blockout_45deg.png", "PERSP",
        (14.0, -22.0, 13.0), target=(0.0, -0.10, 2.15), lens=55.0,
    ))
    renders.append(render_view(
        scene, camera, "RobotCenter_Blockout_Scale.png", "PERSP",
        (7.2, -11.0, 5.9), target=(-0.45, -0.45, 1.85), lens=58.0,
    ))

    # Leave the source file opening on the most useful review angle.
    camera.data.type = "PERSP"
    camera.data.lens = 55.0
    point_camera(camera, (14.0, -22.0, 13.0), (0.0, -0.10, 2.15))
    scene.render.filepath = renders[2]["path"]
    bpy.ops.wm.save_as_mainfile(filepath=TARGET_BLEND, check_existing=False)
    return {"renders": renders, "camera": camera.name, "filepath": bpy.data.filepath}


RENDER_RESULT = render_all()
