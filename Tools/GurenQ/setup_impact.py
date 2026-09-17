"""Configure Q impact presentation and long-range test stations in the existing map."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.material import MaterialTools as MT

E = u.EditorAssetLibrary
R = '/Game/MineLearning/Characters/Guren'
path = R + '/VFX/M_QDashAfterimage'
material = E.load_asset(path)
if not material:
    E.make_directory(R + '/VFX')
    material = u.AssetToolsHelpers.get_asset_tools().create_asset('M_QDashAfterimage', R + '/VFX', u.Material, u.MaterialFactoryNew())
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    color = MT.add_expression(material, u.MaterialExpressionVectorParameter.static_class(), -400, 0)
    color.set_editor_property('parameter_name', 'TrailColor')
    color.set_editor_property('default_value', u.LinearColor(.85, .008, .11, 1))
    opacity = MT.add_expression(material, u.MaterialExpressionScalarParameter.static_class(), -400, 200)
    opacity.set_editor_property('parameter_name', 'Opacity')
    opacity.set_editor_property('default_value', .24)
    u.MaterialEditingLibrary.connect_material_property(color, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    u.MaterialEditingLibrary.connect_material_property(opacity, '', u.MaterialProperty.MP_OPACITY)
    u.MaterialEditingLibrary.set_material_usage(material, u.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    u.MaterialEditingLibrary.recompile_material(material)
    E.save_loaded_asset(material)

bp = E.load_asset(R + '/Blueprints/BPC_GurenQPresentation')
u.get_default_object(bp.generated_class()).set_editor_property('afterimage_material', material)
B.compile_blueprint(bp)
E.save_loaded_asset(bp)
player = E.load_asset(R + '/Blueprints/BP_GurenRetargetTest')
default = u.get_default_object(player.generated_class())
default.get_component_by_class(u.GurenQSkillComponent).set_editor_property('selection_range', 2000)
B.compile_blueprint(player)
E.save_loaded_asset(player)

actors = u.get_editor_subsystem(u.EditorActorSubsystem)
targets = sorted([a for a in actors.get_all_level_actors() if isinstance(a, u.QGrabTestDummy)], key=lambda a: a.get_name())
stations = [('Q Target - Long Dash 1000cm', u.Vector(-1500, 0, 298)),
            ('Q Target - Long Dash 2000cm', u.Vector(500, 2200, 298)),
            ('Q Target - Near Grab', u.Vector(2200, -1400, 298))]
for actor, (label, location) in zip(targets, stations):
    actor.set_actor_label(label)
    actor.set_actor_location(location, False, True)
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('Q_IMPACT_CONFIGURED')
