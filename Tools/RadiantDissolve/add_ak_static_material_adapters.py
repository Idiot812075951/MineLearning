"""Use source-preserving dissolve adapters for the two AK material masters."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
library = u.EditorAssetLibrary
bp = library.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)

variables = {
    'RadiantAKMetalDissolveMaterial': library.load_asset(ROOT + '/M_RadiantDissolve_AKMetalSurface'),
    'RadiantAKPolymerDissolveMaterial': library.load_asset(ROOT + '/M_RadiantDissolve_AKPolymerSurface'),
}
source_variables = {
    'RadiantAKMetalSourceMaterial': library.load_asset('/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Metal'),
    'RadiantAKPolymerSourceMaterial': library.load_asset('/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Polymer'),
}
for name in (*variables, *source_variables):
    if name not in BP.list_variables(bp):
        BP.add_object_variable(bp, name, u.MaterialInterface.static_class())
    BP.set_variable_instance_editable(bp, name, True)
    BP.set_variable_category(bp, name, 'RadiantV2')

BP.compile_blueprint(bp, warnings_as_errors=True)

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantPrepareStatic'), r'''(fn RadiantPrepareStatic ()
 (for mesh (变量|RadiantInternal|GetRadiantStaticMeshes)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantStaticMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (bind source (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (bind sourceBase (Rendering|Material|GetBaseMaterial :self source))
   (bind isPolymer (or (== source (变量|RadiantV2|GetRadiantAKPolymerSourceMaterial)) (== sourceBase (变量|RadiantV2|GetRadiantAKPolymerSourceMaterial))))
   (bind isMetal (or (== source (变量|RadiantV2|GetRadiantAKMetalSourceMaterial)) (== sourceBase (变量|RadiantV2|GetRadiantAKMetalSourceMaterial))))
   (bind polymerOrFallback (工具|Select (变量|RadiantV2|GetRadiantFallbackMaterial) (变量|RadiantV2|GetRadiantAKPolymerDissolveMaterial) isPolymer))
   (bind adapter (工具|Select polymerOrFallback (变量|RadiantV2|GetRadiantAKMetalDissolveMaterial) isMetal))
   (Utilities|Array|Add (变量|RadiantInternal|GetRadiantOriginalStaticMaterials) source)
   (bind mid (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial adapter))
   (Rendering|Material|CopyMaterialInstanceParameters :self mid :Source source :bQuickParametersOnly true))))''')

BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)
for name, material in variables.items():
    cdo.set_editor_property(name, material)
for name, material in source_variables.items():
    cdo.set_editor_property(name, material)
BP.compile_blueprint(bp, warnings_as_errors=True)
library.save_loaded_asset(bp)

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() != 'BP_RadiantDissolve_Test_C':
        continue
    for name, material in variables.items():
        actor.set_editor_property(name, material)
    for name, material in source_variables.items():
        actor.set_editor_property(name, material)

u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_AK_STATIC_ADAPTERS_ADDED')
