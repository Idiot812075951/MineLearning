"""Add a one-time masked-to-translucent handoff for skeletal contour fading."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
library = u.EditorAssetLibrary
bp = library.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)

if 'RadiantSkeletalFadeMaterial' not in BP.list_variables(bp):
    BP.add_object_variable(bp, 'RadiantSkeletalFadeMaterial', u.MaterialInterface.static_class())
if 'RadiantUsingSkeletalFadeMaterial' not in BP.list_variables(bp):
    BP.add_variable(bp, 'RadiantUsingSkeletalFadeMaterial', 'bool')

BP.set_variable_instance_editable(bp, 'RadiantSkeletalFadeMaterial', True)
BP.set_variable_category(bp, 'RadiantSkeletalFadeMaterial', 'RadiantV2')
BP.set_variable_instance_editable(bp, 'RadiantUsingSkeletalFadeMaterial', False)
BP.set_variable_category(bp, 'RadiantUsingSkeletalFadeMaterial', 'RadiantInternal')

try:
    prepare_fade_graph = BP.get_graph(bp, 'RadiantPrepareSkeletalFade')
except Exception:
    prepare_fade_graph = None
if not prepare_fade_graph:
    BP.add_function_graph(bp, 'RadiantPrepareSkeletalFade')
    BP.compile_blueprint(bp)
try:
    update_fade_graph = BP.get_graph(bp, 'RadiantUpdateSkeletalFadeStage')
except Exception:
    update_fade_graph = None
if not update_fade_graph:
    BP.add_function_graph(bp, 'RadiantUpdateSkeletalFadeStage')
    BP.compile_blueprint(bp)

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantPrepareSkeletalFade'), r'''(fn RadiantPrepareSkeletalFade ()
 (for mesh (变量|RadiantInternal|GetRadiantSkeletalMeshes)
  (for slot (range (Rendering|Material|GetNumMaterials :self mesh))
   (bind source (Rendering|Material|GetMaterial :self mesh :ElementIndex slot))
   (bind mid (Rendering|Material|CreateDynamicMaterialInstance :self mesh :ElementIndex slot :SourceMaterial (变量|RadiantV2|GetRadiantSkeletalFadeMaterial)))
   (Rendering|Material|CopyMaterialInstanceParameters :self mid :Source source :bQuickParametersOnly true)))
 (变量|RadiantInternal|SetRadiantUsingSkeletalFadeMaterial true))''')

# Keep the stage decision out of RadiantApplyFrame.  It reads the same normalized
# time (including editor preview mode) and performs a one-time material handoff.
BP.write_graph_dsl(BP.get_graph(bp, 'RadiantUpdateSkeletalFadeStage'), r'''(fn RadiantUpdateSkeletalFadeStage ()
 (bind duration (变量|RadiantV2|GetRadiantDuration))
 (bind safeDuration (工具|Select 0.1 duration (> duration 0.1)))
 (bind elapsedT (/ (变量|RadiantInternal|GetRadiantElapsed) safeDuration))
 (bind rawT (工具|Select elapsedT (变量|RadiantV2|GetRadiantPreviewTime) (变量|RadiantV2|GetRadiantPreviewEnabled)))
 (bind t (工具|Select (工具|Select rawT 1 (> rawT 1)) 0 (< rawT 0)))
 (工具|流程控制|Branch (and (>= t (变量|RadiantV2|GetRadiantOutlineFadeStart)) (not (变量|RadiantInternal|GetRadiantUsingSkeletalFadeMaterial)))
  (:then (CallFunction|RadiantPrepareSkeletalFade))
  (:else (CallFunction|RadiantNoOp))))''')

BP.write_graph_dsl(BP.get_graph(bp, 'RadiantReset'), r'''(fn RadiantReset ()
 (Utilities|Time|ClearTimerbyFunctionName :Object (变量|Getareferencetoself) :FunctionName "RadiantUpdate")
 (for fx (变量|RadiantInternal|GetRadiantActiveParticles)
  (Components|DestroyComponent :self fx))
 (Utilities|Array|Clear (变量|RadiantInternal|GetRadiantActiveParticles))
 (CallFunction|RadiantRestore)
 (变量|RadiantInternal|SetRadiantUsingSkeletalFadeMaterial false)
 (CallFunction|RadiantResolve)
 (CallFunction|RadiantPlaceOrigin)
 (CallFunction|RadiantComputeCoverage)
 (变量|RadiantInternal|SetRadiantElapsed 0)
 (变量|RadiantV2|SetRadiantPreviewEnabled false)
 (CallFunction|RadiantApplyFrame))''')

BP.compile_blueprint(bp, warnings_as_errors=True)
fade_material = library.load_asset(ROOT + '/M_RadiantDissolve_GunnerFade')
cdo = BP.get_default_object(bp)
cdo.set_editor_property('RadiantSkeletalFadeMaterial', fade_material)
BP.compile_blueprint(bp, warnings_as_errors=True)
library.save_loaded_asset(bp)

for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() == 'BP_RadiantDissolve_Test_C':
        actor.set_editor_property('RadiantSkeletalFadeMaterial', fade_material)

print('RADIANT_GUNNER_FADE_STAGE_ADDED')
