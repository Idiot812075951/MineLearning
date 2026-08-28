import bpy
import math
import os
from mathutils import Euler, Matrix, Vector


TARGET_BLEND = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\RobotCenter\RobotCenter.blend"
REFERENCE_IMAGE = r"C:\Users\gh\Documents\Unreal Projects\MineLearning\ArtSource\Environment\RobotCenter\References\RobotCenter_Concept.png"


COLLECTION_NAMES = (
    "00_Reference",
    "01_Base",
    "02_MainStructure",
    "03_RobotShop",
    "04_TransformStation",
    "05_Detail",
)


MATERIAL_SPECS = {
    "M_RC_Yellow": {
        "color": (0.72, 0.30, 0.035, 1.0),
        "metallic": 0.08,
        "roughness": 0.42,
    },
    "M_RC_Gunmetal": {
        "color": (0.045, 0.055, 0.070, 1.0),
        "metallic": 0.72,
        "roughness": 0.34,
    },
    "M_RC_Silver": {
        "color": (0.34, 0.40, 0.47, 1.0),
        "metallic": 0.86,
        "roughness": 0.25,
    },
    "M_RC_DarkRubber": {
        "color": (0.018, 0.024, 0.032, 1.0),
        "metallic": 0.0,
        "roughness": 0.82,
    },
    "M_RC_Cyan": {
        "color": (0.00, 0.42, 0.72, 1.0),
        "metallic": 0.08,
        "roughness": 0.24,
        "emission": (0.00, 0.55, 1.00, 1.0),
        "emission_strength": 2.2,
    },
    "M_RC_Orange": {
        "color": (0.82, 0.16, 0.025, 1.0),
        "metallic": 0.0,
        "roughness": 0.46,
    },
    "M_RC_ProxyHuman": {
        "color": (0.10, 0.34, 0.85, 0.55),
        "metallic": 0.0,
        "roughness": 0.55,
    },
    "M_RC_ProxyRobot": {
        "color": (0.22, 0.74, 0.32, 0.55),
        "metallic": 0.0,
        "roughness": 0.55,
    },
    "M_RC_PreviewFloor": {
        "color": (0.025, 0.032, 0.043, 1.0),
        "metallic": 0.0,
        "roughness": 0.78,
    },
}


class MeshAccumulator:
    def __init__(self):
        self.verts = []
        self.faces = []
        self.face_mats = []

    def _append(self, verts, faces, material_index):
        offset = len(self.verts)
        self.verts.extend(verts)
        self.faces.extend([[offset + i for i in face] for face in faces])
        self.face_mats.extend([material_index] * len(faces))

    def add_box(self, center, dimensions, material_index, rotation=(0.0, 0.0, 0.0)):
        hx, hy, hz = [v * 0.5 for v in dimensions]
        local = [
            (-hx, -hy, -hz), (hx, -hy, -hz), (hx, hy, -hz), (-hx, hy, -hz),
            (-hx, -hy, hz), (hx, -hy, hz), (hx, hy, hz), (-hx, hy, hz),
        ]
        matrix = Matrix.Translation(Vector(center)) @ Euler(rotation).to_matrix().to_4x4()
        verts = [tuple(matrix @ Vector(v)) for v in local]
        faces = [
            (0, 3, 2, 1), (4, 5, 6, 7),
            (0, 1, 5, 4), (1, 2, 6, 5),
            (2, 3, 7, 6), (3, 0, 4, 7),
        ]
        self._append(verts, faces, material_index)

    def add_chamfered_prism(self, center_xy, width, depth, z0, z1, chamfer, material_index):
        cx, cy = center_xy
        hx, hy = width * 0.5, depth * 0.5
        c = min(chamfer, hx * 0.45, hy * 0.45)
        ring = [
            (-hx + c, -hy), (hx - c, -hy), (hx, -hy + c), (hx, hy - c),
            (hx - c, hy), (-hx + c, hy), (-hx, hy - c), (-hx, -hy + c),
        ]
        verts = [(cx + x, cy + y, z0) for x, y in ring] + [(cx + x, cy + y, z1) for x, y in ring]
        faces = [tuple(reversed(range(8))), tuple(range(8, 16))]
        faces.extend((i, (i + 1) % 8, 8 + (i + 1) % 8, 8 + i) for i in range(8))
        self._append(verts, faces, material_index)

    def add_cylinder(self, center_xy, radius, z0, z1, material_index, segments=32):
        cx, cy = center_xy
        verts = []
        for z in (z0, z1):
            for i in range(segments):
                a = math.tau * i / segments
                verts.append((cx + radius * math.cos(a), cy + radius * math.sin(a), z))
        faces = [tuple(reversed(range(segments))), tuple(range(segments, segments * 2))]
        faces.extend((i, (i + 1) % segments, segments + (i + 1) % segments, segments + i) for i in range(segments))
        self._append(verts, faces, material_index)

    def add_ring(self, center_xy, outer_radius, inner_radius, z0, z1, material_index, segments=40):
        cx, cy = center_xy
        verts = []
        for z in (z0, z1):
            for radius in (outer_radius, inner_radius):
                for i in range(segments):
                    a = math.tau * i / segments
                    verts.append((cx + radius * math.cos(a), cy + radius * math.sin(a), z))
        ob, ib, ot, it = 0, segments, segments * 2, segments * 3
        faces = []
        for i in range(segments):
            j = (i + 1) % segments
            faces.extend([
                (ob + i, ob + j, ot + j, ot + i),
                (ib + j, ib + i, it + i, it + j),
                (ot + i, ot + j, it + j, it + i),
                (ob + j, ob + i, ib + i, ib + j),
            ])
        self._append(verts, faces, material_index)

    def add_arch(self, center_xz, y_center, outer_radius, inner_radius, depth, material_index, segments=28):
        cx, cz = center_xz
        y_front = y_center - depth * 0.5
        y_back = y_center + depth * 0.5
        verts = []
        for y in (y_front, y_back):
            for radius in (outer_radius, inner_radius):
                for i in range(segments + 1):
                    angle = math.pi * i / segments
                    verts.append((cx + radius * math.cos(angle), y, cz + radius * math.sin(angle)))
        count = segments + 1
        of, inf, ob, inb = 0, count, count * 2, count * 3
        faces = []
        for i in range(segments):
            j = i + 1
            faces.extend([
                (of + i, of + j, inf + j, inf + i),
                (ob + j, ob + i, inb + i, inb + j),
                (of + j, of + i, ob + i, ob + j),
                (inf + i, inf + j, inb + j, inb + i),
            ])
        faces.extend([
            (of, inf, inb, ob),
            (of + segments, ob + segments, inb + segments, inf + segments),
        ])
        self._append(verts, faces, material_index)

    def add_ramp_wedge(self, width, y_front, y_back, z_front, z_back, material_index):
        hx = width * 0.5
        verts = [
            (-hx, y_front, 0.0), (hx, y_front, 0.0), (hx, y_back, 0.0), (-hx, y_back, 0.0),
            (-hx, y_front, z_front), (hx, y_front, z_front), (hx, y_back, z_back), (-hx, y_back, z_back),
        ]
        faces = [
            (0, 3, 2, 1), (4, 5, 6, 7),
            (0, 1, 5, 4), (1, 2, 6, 5),
            (2, 3, 7, 6), (3, 0, 4, 7),
        ]
        self._append(verts, faces, material_index)

    def add_oriented_ring(self, center, outer_radius, inner_radius, depth, direction, material_index, segments=28):
        half_depth = depth * 0.5
        verts = []
        for z in (-half_depth, half_depth):
            for radius in (outer_radius, inner_radius):
                for i in range(segments):
                    angle = math.tau * i / segments
                    verts.append((radius * math.cos(angle), radius * math.sin(angle), z))
        ob, ib, ot, it = 0, segments, segments * 2, segments * 3
        faces = []
        for i in range(segments):
            j = (i + 1) % segments
            faces.extend([
                (ob + i, ob + j, ot + j, ot + i),
                (ib + j, ib + i, it + i, it + j),
                (ot + i, ot + j, it + j, it + i),
                (ob + j, ob + i, ib + i, ib + j),
            ])
        axis = Vector(direction).normalized()
        matrix = Matrix.Translation(Vector(center)) @ axis.to_track_quat("Z", "Y").to_matrix().to_4x4()
        self._append([tuple(matrix @ Vector(v)) for v in verts], faces, material_index)

    def transform(self, matrix):
        self.verts = [tuple(matrix @ Vector(vertex)) for vertex in self.verts]


def remove_existing_scene_content():
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for collection in list(bpy.data.collections):
        bpy.data.collections.remove(collection)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights, bpy.data.materials):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def make_collections(scene):
    root = bpy.data.collections.new("RobotCenter")
    scene.collection.children.link(root)
    collections = {}
    for name in COLLECTION_NAMES:
        collection = bpy.data.collections.new(name)
        root.children.link(collection)
        collections[name] = collection
    return root, collections


def make_material(name, spec):
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    material.diffuse_color = spec["color"]
    material.metallic = spec.get("metallic", 0.0)
    material.roughness = spec.get("roughness", 0.5)
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        if bsdf.inputs.get("Base Color"):
            bsdf.inputs["Base Color"].default_value = spec["color"]
        if bsdf.inputs.get("Metallic"):
            bsdf.inputs["Metallic"].default_value = spec.get("metallic", 0.0)
        if bsdf.inputs.get("Roughness"):
            bsdf.inputs["Roughness"].default_value = spec.get("roughness", 0.5)
        emission = spec.get("emission")
        if emission:
            emission_input = bsdf.inputs.get("Emission Color") or bsdf.inputs.get("Emission")
            strength_input = bsdf.inputs.get("Emission Strength")
            if emission_input:
                emission_input.default_value = emission
            if strength_input:
                strength_input.default_value = spec.get("emission_strength", 1.0)
    return material


def create_mesh_object(name, accumulator, collection, materials, material_order, origin, bevel=0.08):
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(accumulator.verts, [], accumulator.faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    used_material_indices = sorted(set(accumulator.face_mats))
    local_material_indices = {global_index: local_index for local_index, global_index in enumerate(used_material_indices)}
    for global_index in used_material_indices:
        mesh.materials.append(materials[material_order[global_index]])
    for polygon, global_material_index in zip(mesh.polygons, accumulator.face_mats):
        polygon.material_index = local_material_indices[global_material_index]
    origin_vec = Vector(origin)
    mesh.transform(Matrix.Translation(-origin_vec))
    obj.location = origin_vec
    obj["RC_Phase"] = "P02_StructureOptimization"
    obj["RC_ExportCandidate"] = True
    if bevel > 0.0:
        modifier = obj.modifiers.new("Bevel_Blockout", "BEVEL")
        modifier.width = bevel
        modifier.segments = 3
        modifier.limit_method = "ANGLE"
    return obj


def create_curve(name, points, collection, material, bevel_depth=0.12):
    curve = bpy.data.curves.new(f"{name}_Curve", "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 3
    curve.bevel_depth = bevel_depth
    curve.bevel_resolution = 3
    curve.resolution_u = 12
    spline = curve.splines.new("BEZIER")
    spline.bezier_points.add(len(points) - 1)
    for point, coordinate in zip(spline.bezier_points, points):
        point.co = coordinate
        point.handle_left_type = "AUTO"
        point.handle_right_type = "AUTO"
    obj = bpy.data.objects.new(name, curve)
    collection.objects.link(obj)
    curve.materials.append(material)
    obj["RC_Phase"] = "P02_StructureOptimization"
    obj["RC_ExportCandidate"] = True
    return obj


def create_empty(name, location, collection, size=0.24, display_type="ARROWS"):
    obj = bpy.data.objects.new(name, None)
    collection.objects.link(obj)
    obj.location = location
    obj.empty_display_type = display_type
    obj.empty_display_size = size
    obj.show_name = True
    obj["RC_ExportCandidate"] = False
    return obj


def create_reference_proxies(collection, materials):
    human_acc = MeshAccumulator()
    human_acc.add_cylinder((0.0, -2.15), 0.26, 0.78, 2.25, 0, segments=20)
    human_acc.add_cylinder((0.0, -2.15), 0.34, 2.25, 2.58, 0, segments=20)
    human = create_mesh_object(
        "REF_Human_1p8m", human_acc, collection, materials,
        ["M_RC_ProxyHuman"], (0.0, -2.15, 0.78), bevel=0.06,
    )
    human.display_type = "WIRE"
    human.show_in_front = True
    human.hide_render = False
    human["RC_ExportCandidate"] = False
    human["ReferenceHeightMeters"] = 1.8

    robot_acc = MeshAccumulator()
    robot_acc.add_box((-2.55, -1.55, 1.79), (0.70, 0.62, 0.72), 0)
    robot_acc.add_box((-2.55, -1.55, 2.32), (0.56, 0.50, 0.42), 0)
    robot = create_mesh_object(
        "REF_OreBuddy_1p1m", robot_acc, collection, materials,
        ["M_RC_ProxyRobot"], (-2.55, -1.55, 1.43), bevel=0.09,
    )
    robot.display_type = "WIRE"
    robot.show_in_front = True
    robot.hide_render = False
    robot["RC_ExportCandidate"] = False
    robot["ReferenceHeightMeters"] = 1.1


def create_preview_setup(scene, collection, materials):
    floor_acc = MeshAccumulator()
    floor_acc.add_cylinder((0.0, 0.0), 11.0, -0.12, -0.08, 0, segments=64)
    floor = create_mesh_object(
        "REF_PreviewFloor", floor_acc, collection, materials,
        ["M_RC_PreviewFloor"], (0.0, 0.0, -0.12), bevel=0.0,
    )
    floor["RC_ExportCandidate"] = False

    camera_data = bpy.data.cameras.new("PREVIEW_Camera_Data")
    camera = bpy.data.objects.new("PREVIEW_Camera", camera_data)
    collection.objects.link(camera)
    camera_data.lens = 52
    camera_data.sensor_width = 36
    camera["RC_ExportCandidate"] = False
    scene.camera = camera

    light_specs = [
        ("PREVIEW_Key", (4.5, -7.0, 10.0), 1350.0, 6.0, (1.0, 0.79, 0.58)),
        ("PREVIEW_Fill", (-7.0, -3.0, 6.0), 900.0, 5.0, (0.50, 0.70, 1.0)),
        ("PREVIEW_Rim", (2.0, 6.5, 8.0), 1100.0, 4.0, (0.35, 0.68, 1.0)),
    ]
    for name, location, energy, size, color in light_specs:
        data = bpy.data.lights.new(f"{name}_Data", "AREA")
        data.energy = energy
        data.shape = "DISK"
        data.size = size
        data.color = color
        obj = bpy.data.objects.new(name, data)
        collection.objects.link(obj)
        obj.location = location
        obj["RC_ExportCandidate"] = False
        direction = Vector((0.0, 0.0, 1.8)) - obj.location
        obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def build():
    if os.path.normcase(os.path.abspath(bpy.data.filepath)) != os.path.normcase(os.path.abspath(TARGET_BLEND)):
        raise RuntimeError(f"Wrong Blender file connected: {bpy.data.filepath!r}")

    remove_existing_scene_content()
    scene = bpy.context.scene
    scene.name = "RobotCenter_P02_Structure"
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.unit_settings.length_unit = "METERS"
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1100
    scene.render.resolution_y = 825
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.world.color = (0.012, 0.017, 0.026)

    root, collections = make_collections(scene)
    materials = {name: make_material(name, spec) for name, spec in MATERIAL_SPECS.items()}
    material_order = list(MATERIAL_SPECS.keys())
    mat_index = {name: i for i, name in enumerate(material_order)}

    # 01 Main Frame: heavier header, broader pillars, thicker bases, two complete end wraps.
    frame_acc = MeshAccumulator()
    for x in (-5.55, 5.55):
        frame_acc.add_box((x, 2.35, 1.12), (1.68, 1.52, 0.68), mat_index["M_RC_Gunmetal"])
        frame_acc.add_box((x, 2.35, 2.72), (1.12, 1.08, 3.05), mat_index["M_RC_Gunmetal"])
        frame_acc.add_box((x, 2.12, 2.72), (0.38, 1.34, 2.20), mat_index["M_RC_Silver"])
        frame_acc.add_box((x, 2.30, 4.63), (1.68, 1.36, 1.38), mat_index["M_RC_Yellow"])
    frame_acc.add_box((0.0, 2.34, 4.63), (11.85, 1.22, 1.18), mat_index["M_RC_Gunmetal"])
    frame_acc.add_box((0.0, 1.66, 4.64), (5.70, 0.26, 0.84), mat_index["M_RC_Yellow"])
    frame_acc.add_box((0.0, 1.49, 4.64), (5.08, 0.16, 0.58), mat_index["M_RC_Gunmetal"])
    frame_acc.add_box((0.0, 1.38, 4.64), (3.15, 0.06, 0.20), mat_index["M_RC_Cyan"])
    create_mesh_object("SM_RobotCenter_MainFrame", frame_acc, collections["02_MainStructure"], materials, material_order, (0.0, 2.35, 0.78), bevel=0.12)

    # 02 Main Core: broader, taller, forward, with one bounded primary display and a heavy base.
    core_acc = MeshAccumulator()
    core_acc.add_chamfered_prism((0.0, 1.38), 3.55, 2.15, 0.78, 1.22, 0.28, mat_index["M_RC_Gunmetal"])
    core_acc.add_chamfered_prism((0.0, 1.42), 3.12, 1.86, 1.16, 3.48, 0.24, mat_index["M_RC_Gunmetal"])
    core_acc.add_box((0.0, 1.42, 3.55), (3.45, 2.02, 0.54), mat_index["M_RC_Yellow"])
    core_acc.add_box((0.0, 0.42, 2.34), (2.26, 0.18, 1.24), mat_index["M_RC_Gunmetal"])
    core_acc.add_box((0.0, 0.30, 2.34), (1.58, 0.08, 0.68), mat_index["M_RC_Cyan"])
    core_acc.add_box((0.0, 0.37, 1.22), (2.24, 0.20, 0.36), mat_index["M_RC_Silver"])
    # Exactly two short supports connect the core to the frame.
    core_acc.add_box((-1.05, 1.92, 3.93), (0.38, 0.68, 1.08), mat_index["M_RC_Gunmetal"], rotation=(0.0, -0.34, 0.0))
    core_acc.add_box((1.05, 1.92, 3.93), (0.38, 0.68, 1.08), mat_index["M_RC_Gunmetal"], rotation=(0.0, 0.34, 0.0))
    # Two system interface collars, one for each approved main link.
    for x, direction in ((-1.62, (-1.0, 0.0, 0.0)), (1.62, (1.0, 0.0, 0.0))):
        core_acc.add_oriented_ring((x, 1.15, 1.72), 0.32, 0.20, 0.20, direction, mat_index["M_RC_Gunmetal"])
        core_acc.add_oriented_ring((x, 1.15, 1.72), 0.20, 0.13, 0.24, direction, mat_index["M_RC_Cyan"])
    create_mesh_object("SM_RobotCenter_MainCore", core_acc, collections["02_MainStructure"], materials, material_order, (0.0, 1.38, 0.78), bevel=0.12)

    # 03 Shop Terminal: modestly broader, still below the core, and aimed toward the display pad.
    shop_acc = MeshAccumulator()
    shop_center = Vector((-4.15, 0.32, 0.0))
    shop_acc.add_chamfered_prism((-4.15, 0.32), 3.05, 1.96, 0.78, 1.16, 0.26, mat_index["M_RC_Gunmetal"])
    shop_acc.add_chamfered_prism((-4.15, 0.32), 2.78, 1.76, 1.10, 2.62, 0.24, mat_index["M_RC_Gunmetal"])
    shop_acc.add_box((-4.15, 0.30, 2.69), (3.04, 1.92, 0.42), mat_index["M_RC_Yellow"])
    shop_acc.add_box((-4.15, -0.61, 2.12), (1.98, 0.16, 0.94), mat_index["M_RC_Gunmetal"])
    shop_acc.add_box((-4.15, -0.72, 2.12), (1.42, 0.07, 0.62), mat_index["M_RC_Cyan"])
    shop_acc.add_box((-4.15, -0.69, 1.34), (1.56, 0.64, 0.46), mat_index["M_RC_Yellow"], rotation=(-0.20, 0.0, 0.0))
    shop_acc.add_box((-4.15, -0.98, 1.48), (1.14, 0.12, 0.24), mat_index["M_RC_Gunmetal"], rotation=(-0.20, 0.0, 0.0))
    shop_rotation = Matrix.Translation(shop_center) @ Matrix.Rotation(math.radians(8.0), 4, "Z") @ Matrix.Translation(-shop_center)
    shop_acc.transform(shop_rotation)
    create_mesh_object("SM_RobotCenter_ShopTerminal", shop_acc, collections["03_RobotShop"], materials, material_order, (-4.15, 0.32, 0.78), bevel=0.11)

    # 04 Shop Display: 25% larger, one light border, four large locators, one shared connector.
    shop_pad_acc = MeshAccumulator()
    shop_pad_acc.add_chamfered_prism((-2.55, -1.55), 3.18, 2.78, 0.78, 1.08, 0.34, mat_index["M_RC_Gunmetal"])
    shop_pad_acc.add_chamfered_prism((-2.55, -1.55), 2.90, 2.50, 1.08, 1.27, 0.30, mat_index["M_RC_Yellow"])
    shop_pad_acc.add_chamfered_prism((-2.55, -1.55), 2.48, 2.08, 1.27, 1.35, 0.24, mat_index["M_RC_Cyan"])
    shop_pad_acc.add_chamfered_prism((-2.55, -1.55), 2.20, 1.80, 1.35, 1.43, 0.20, mat_index["M_RC_Gunmetal"])
    for x in (-3.78, -1.32):
        for y in (-2.58, -0.52):
            shop_pad_acc.add_box((x, y, 1.49), (0.38, 0.38, 0.24), mat_index["M_RC_Gunmetal"])
    shop_pad_acc.add_box((-3.33, -0.46, 1.08), (1.16, 0.42, 0.24), mat_index["M_RC_Gunmetal"])
    create_mesh_object("SM_RobotCenter_ShopDisplayPad", shop_pad_acc, collections["03_RobotShop"], materials, material_order, (-2.55, -1.55, 0.78), bevel=0.08)

    # 05 Form Pad: one clean cyan ring, a stronger mechanical ring, and exactly three large clamps.
    form_center = (3.32, -1.12)
    form_pad_acc = MeshAccumulator()
    form_pad_acc.add_cylinder(form_center, 1.98, 0.78, 1.02, mat_index["M_RC_Gunmetal"], segments=44)
    form_pad_acc.add_ring(form_center, 1.84, 1.38, 1.02, 1.23, mat_index["M_RC_Yellow"], segments=44)
    form_pad_acc.add_ring(form_center, 1.38, 1.12, 1.04, 1.31, mat_index["M_RC_Cyan"], segments=44)
    form_pad_acc.add_cylinder(form_center, 1.12, 1.04, 1.34, mat_index["M_RC_Gunmetal"], segments=44)
    form_pad_acc.add_cylinder(form_center, 0.26, 1.34, 1.40, mat_index["M_RC_Cyan"], segments=28)
    for angle_deg in (90.0, 210.0, 330.0):
        angle = math.radians(angle_deg)
        x = form_center[0] + 1.82 * math.cos(angle)
        y = form_center[1] + 1.82 * math.sin(angle)
        form_pad_acc.add_box((x, y, 1.43), (0.58, 0.40, 0.28), mat_index["M_RC_Gunmetal"], rotation=(0.0, 0.0, angle))
    create_mesh_object("SM_RobotCenter_FormPad", form_pad_acc, collections["04_TransformStation"], materials, material_order, (3.32, -1.12, 0.78), bevel=0.06)

    # 06 Scanner: co-axial with the pad, one dark inner ring, and exactly three scanner nodes.
    scanner_acc = MeshAccumulator()
    scanner_acc.add_arch((3.32, 2.30), -1.12, 2.10, 1.62, 0.82, mat_index["M_RC_Yellow"], segments=32)
    scanner_acc.add_arch((3.32, 2.30), -1.12, 1.62, 1.34, 0.88, mat_index["M_RC_Gunmetal"], segments=32)
    for x in (1.54, 5.10):
        scanner_acc.add_box((x, -1.12, 1.55), (0.82, 0.94, 1.54), mat_index["M_RC_Gunmetal"])
        scanner_acc.add_box((x, -1.18, 2.18), (0.98, 1.02, 0.46), mat_index["M_RC_Yellow"])
    # Top node points down; the two low nodes point inward toward the player axis.
    scanner_acc.add_box((3.32, -1.60, 4.36), (0.74, 0.38, 0.50), mat_index["M_RC_Gunmetal"])
    scanner_acc.add_box((3.32, -1.60, 4.08), (0.38, 0.26, 0.08), mat_index["M_RC_Cyan"])
    scanner_acc.add_box((1.63, -1.60, 1.78), (0.50, 0.40, 0.56), mat_index["M_RC_Gunmetal"])
    scanner_acc.add_box((1.91, -1.60, 1.78), (0.08, 0.26, 0.24), mat_index["M_RC_Cyan"])
    scanner_acc.add_box((5.01, -1.60, 1.78), (0.50, 0.40, 0.56), mat_index["M_RC_Gunmetal"])
    scanner_acc.add_box((4.73, -1.60, 1.78), (0.08, 0.26, 0.24), mat_index["M_RC_Cyan"])
    create_mesh_object("SM_RobotCenter_FormScanner", scanner_acc, collections["04_TransformStation"], materials, material_order, (3.32, -1.12, 0.78), bevel=0.08)

    # 07 Form Terminal: stronger but clearly secondary to the Shop Terminal and Scanner.
    form_terminal_acc = MeshAccumulator()
    form_terminal_acc.add_chamfered_prism((5.72, -2.18), 1.10, 1.00, 0.78, 1.72, 0.14, mat_index["M_RC_Gunmetal"])
    form_terminal_acc.add_box((5.72, -2.20, 1.74), (1.22, 1.10, 0.38), mat_index["M_RC_Yellow"], rotation=(-0.18, 0.0, 0.0))
    form_terminal_acc.add_box((5.72, -2.70, 1.83), (0.60, 0.09, 0.28), mat_index["M_RC_Cyan"], rotation=(-0.18, 0.0, 0.0))
    create_mesh_object("SM_RobotCenter_FormTerminal", form_terminal_acc, collections["04_TransformStation"], materials, material_order, (5.72, -2.18, 0.78), bevel=0.08)

    # 08 Exactly two thicker, readable system links. No third cable is introduced.
    create_curve(
        "SM_RobotCenter_Link_Shop",
        [(-1.72, 1.15, 1.72), (-2.12, 0.95, 1.55), (-2.72, 0.58, 1.38), (-3.10, 0.05, 1.40)],
        collections["05_Detail"], materials["M_RC_DarkRubber"], bevel_depth=0.17,
    )
    create_curve(
        "SM_RobotCenter_Link_Transform",
        [(1.72, 1.15, 1.72), (2.02, 0.82, 1.56), (1.98, -0.02, 1.42), (1.62, -0.72, 1.46)],
        collections["05_Detail"], materials["M_RC_DarkRubber"], bevel_depth=0.17,
    )

    # 09 Base remains intentionally simple; only the approved logistics ramp changes.
    base_acc = MeshAccumulator()
    base_acc.add_chamfered_prism((0.0, 0.0), 14.0, 7.0, 0.0, 0.48, 1.0, mat_index["M_RC_Gunmetal"])
    base_acc.add_chamfered_prism((0.0, 0.0), 13.55, 6.55, 0.48, 0.66, 0.88, mat_index["M_RC_Yellow"])
    base_acc.add_chamfered_prism((0.0, 0.0), 12.85, 5.90, 0.66, 0.78, 0.72, mat_index["M_RC_Gunmetal"])
    create_mesh_object("SM_RobotCenter_Base", base_acc, collections["01_Base"], materials, material_order, (0.0, 0.0, 0.0), bevel=0.11)

    ramp_acc = MeshAccumulator()
    ramp_acc.add_ramp_wedge(4.32, -5.60, -3.08, 0.10, 0.78, mat_index["M_RC_Gunmetal"])
    slope_length = math.sqrt((5.60 - 3.08) ** 2 + (0.78 - 0.10) ** 2)
    slope_angle = math.atan2(0.78 - 0.10, 5.60 - 3.08)
    for x in (-2.07, 2.07):
        ramp_acc.add_box((x, -4.34, 0.44), (0.16, slope_length, 0.14), mat_index["M_RC_Yellow"], rotation=(slope_angle, 0.0, 0.0))
    create_mesh_object("SM_RobotCenter_AccessRamp", ramp_acc, collections["01_Base"], materials, material_order, (0.0, -5.60, 0.0), bevel=0.06)

    create_reference_proxies(collections["00_Reference"], materials)
    create_empty("ShopInteractPoint", (-4.05, -1.12, 0.82), collections["00_Reference"])
    create_empty("FormInteractPoint", (4.98, -2.30, 0.82), collections["00_Reference"])
    create_empty("ShopRobotDisplayPoint", (-2.55, -1.55, 1.43), collections["00_Reference"])
    create_empty("FormPlayerStandPoint", (3.32, -1.12, 1.40), collections["00_Reference"])
    create_empty("FormFXCenter", (3.32, -1.12, 2.00), collections["00_Reference"])
    create_preview_setup(scene, collections["00_Reference"], materials)

    root["RC_Asset"] = "Robot Center"
    root["RC_Phase"] = "P02_StructureOptimization"
    root["RC_IdentityColor"] = "Engineering Yellow"
    root["RC_Function"] = "Open industrial robot shop and player transformation facility"
    root["RC_FrontAxis"] = "-Y"
    root["RC_SourceOfTruth"] = "RobotCenter.blend"
    root["RC_ReferenceImage"] = REFERENCE_IMAGE

    bpy.context.view_layer.objects.active = None
    for obj in bpy.context.selected_objects:
        obj.select_set(False)
    bpy.ops.wm.save_as_mainfile(filepath=TARGET_BLEND, check_existing=False)

    export_candidates = [obj.name for obj in bpy.data.objects if obj.get("RC_ExportCandidate") is True]
    return {
        "filepath": bpy.data.filepath,
        "scene": scene.name,
        "phase": root["RC_Phase"],
        "collections": [c.name for c in root.children],
        "object_count": len(bpy.data.objects),
        "mesh_count": len(bpy.data.meshes),
        "export_candidates": export_candidates,
        "materials": list(MATERIAL_SPECS.keys()),
        "unit_system": scene.unit_settings.system,
        "unit_scale": scene.unit_settings.scale_length,
    }


BUILD_RESULT = build()
