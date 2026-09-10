"""Copy source material-instance parameters into dissolve adapter MIDs."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
library = u.EditorAssetLibrary
bp = library.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)


def write(name, body):
    BP.write_graph_dsl(BP.get_graph(bp, name), body)


write('RadiantPrepareStatic', r'''(fn RadiantPrepareStatic ()
 (for mesh (变量|RadiantInternal|GetRadiantStaticMeshes)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantStaticMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (bind source (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (Utilities|Array|Add (变量|RadiantInternal|GetRadiantOriginalStaticMaterials) source)
   (bind mid (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|RadiantV2|GetRadiantFallbackMaterial)))
   (Rendering|Material|CopyMaterialInstanceParameters :self mid :Source source :bQuickParametersOnly true))))''')

write('RadiantPrepareSkeletal', r'''(fn RadiantPrepareSkeletal ()
 (for mesh (变量|RadiantInternal|GetRadiantSkeletalMeshes)
  (Utilities|Array|Add (变量|RadiantInternal|GetRadiantSkeletalMaterialCounts) (Rendering|Material|GetNumMaterials :self mesh))
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (bind source (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (Utilities|Array|Add (变量|RadiantInternal|GetRadiantOriginalSkeletalMaterials) source)
   (bind mid (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|RadiantV2|GetRadiantSkeletalFallbackMaterial)))
   (Rendering|Material|CopyMaterialInstanceParameters :self mid :Source source :bQuickParametersOnly true))))''')

BP.compile_blueprint(bp, warnings_as_errors=True)
library.save_loaded_asset(bp)
print('RADIANT_SOURCE_PARAMETERS_PRESERVED')
