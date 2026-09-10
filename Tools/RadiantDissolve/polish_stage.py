import unreal as u
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.get_current_level().get_outer().get_path_name()==ROOT+'/L_RadiantDissolve_Test.L_RadiantDissolve_Test'
E=u.EditorAssetLibrary; M=u.MaterialEditingLibrary
for name,color in [('M_DemoStage',(.018,.025,.033)),('M_DemoFloor',(.009,.012,.018))]:
    mat=E.load_asset(ROOT+'/'+name) if E.does_asset_exist(ROOT+'/'+name) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,ROOT,u.Material,u.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    c=M.create_material_expression(mat,u.MaterialExpressionConstant3Vector,-250,0);c.set_editor_property('constant',u.LinearColor(*color,1))
    r=M.create_material_expression(mat,u.MaterialExpressionConstant,-250,150);r.set_editor_property('r',.8)
    M.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR);M.connect_material_property(r,'',u.MaterialProperty.MP_ROUGHNESS)
    M.recompile_material(mat);E.save_loaded_asset(mat)
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
for a in actors:
    if a.get_actor_label()=='NeutralStage': a.static_mesh_component.set_material(0,E.load_asset(ROOT+'/M_DemoStage'))
    if a.get_actor_label()=='StudioFloor': a.static_mesh_component.set_material(0,E.load_asset(ROOT+'/M_DemoFloor'))
    if a.get_actor_label()=='DemoCamera':
        a.camera_component.set_field_of_view(32)
    if a.get_actor_label()=='SoftFill': a.light_component.set_intensity(2400)
    if a.get_actor_label()=='RadiantDissolve_Test':
        a.set_editor_property('PreviewTime',0.)
        a.call_method('Reset')
        print('ORIGIN',a.get_editor_property('DissolveOrigin').get_world_location())
level.save_current_level()
print('RADIANT_STAGE_POLISHED')
