"""Add collision-free Blueprint names for the reusable radiant dissolve controller."""
import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP, ContainerType

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
bp = u.load_asset(ROOT + '/BP_RadiantDissolve_Test')
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(bp)


def add_value(name, type_name, array=False):
    if name not in BP.list_variables(bp):
        BP.add_variable(
            bp,
            name,
            type_name,
            container_type=ContainerType.ARRAY if array else None,
        )


def add_object(name, object_class, array=False):
    if name not in BP.list_variables(bp):
        BP.add_object_variable(
            bp,
            name,
            object_class.static_class(),
            container_type=ContainerType.ARRAY if array else None,
        )


# Instance-facing controls. The Radiant prefix prevents Blueprint node lookup from
# resolving common names such as Duration or TargetActor to unrelated plugins.
add_object('RadiantTarget', u.Actor)
add_object('RadiantFallbackMaterial', u.MaterialInterface)
add_object('RadiantSkeletalFallbackMaterial', u.MaterialInterface)
for name in ['RadiantAutoPlaceOrigin', 'RadiantShowOriginMarker', 'RadiantPreviewEnabled']:
    add_value(name, 'bool')
for name in [
    'RadiantOriginHeightRatio', 'RadiantDuration', 'RadiantTravelDistance',
    'RadiantNoiseStrength', 'RadiantHeatIntensity', 'RadiantEdgeWidth',
    'RadiantDissolveStart', 'RadiantOutlineHoldStart',
    'RadiantOutlineFadeStart', 'RadiantMeshGone', 'RadiantParticleRate',
    'RadiantParticleFrontWidth', 'RadiantPreviewTime',
]:
    add_value(name, 'float')

# Runtime bookkeeping. Static and skeletal state is intentionally separated so
# Reset restores every source material slot in deterministic order.
add_value('RadiantElapsed', 'float')
add_value('RadiantStartTime', 'float')
add_value('RadiantEffectiveRadius', 'float')
add_value('RadiantMaterialCursor', 'int')
add_value('RadiantStaticMaterialCounts', 'int', True)
add_value('RadiantSkeletalMaterialCounts', 'int', True)
add_object('RadiantStaticMeshes', u.StaticMeshComponent, True)
add_object('RadiantSkeletalMeshes', u.SkeletalMeshComponent, True)
add_object('RadiantOriginalStaticMaterials', u.MaterialInterface, True)
add_object('RadiantOriginalSkeletalMaterials', u.MaterialInterface, True)
add_object('RadiantActiveParticles', u.NiagaraComponent, True)

public_names = [
    'RadiantTarget', 'RadiantFallbackMaterial', 'RadiantSkeletalFallbackMaterial', 'RadiantAutoPlaceOrigin',
    'RadiantShowOriginMarker', 'RadiantOriginHeightRatio', 'RadiantDuration',
    'RadiantTravelDistance', 'RadiantNoiseStrength', 'RadiantHeatIntensity',
    'RadiantEdgeWidth', 'RadiantDissolveStart', 'RadiantOutlineHoldStart',
    'RadiantOutlineFadeStart', 'RadiantMeshGone', 'RadiantParticleRate',
    'RadiantParticleFrontWidth', 'RadiantPreviewEnabled', 'RadiantPreviewTime',
]
for name in public_names:
    BP.set_variable_instance_editable(bp, name, True)
    BP.set_variable_category(bp, name, 'RadiantV2')

internal_names = [
    'RadiantElapsed', 'RadiantStartTime', 'RadiantEffectiveRadius',
    'RadiantMaterialCursor', 'RadiantStaticMaterialCounts',
    'RadiantSkeletalMaterialCounts', 'RadiantStaticMeshes',
    'RadiantSkeletalMeshes', 'RadiantOriginalStaticMaterials',
    'RadiantOriginalSkeletalMaterials', 'RadiantActiveParticles',
]
for name in internal_names:
    BP.set_variable_instance_editable(bp, name, False)
    BP.set_variable_category(bp, name, 'RadiantInternal')

# Hide the superseded prototype controls. They remain only so the old graphs can
# compile until the final graph rewrite below is saved.
for name in [
    'TargetActor', 'FallbackMaterial', 'AutoPlaceOrigin', 'OriginHeightRatio',
    'Duration', 'PropagationDistance', 'NoiseStrength', 'HeatIntensity',
    'DissolveEdgeWidth', 'ParticleSampleRate', 'ParticleFrontWidth',
    'DissolveStartFraction', 'OutlineHoldStartFraction',
    'OutlineFadeStartFraction', 'MeshGoneFraction', 'PreviewEnabled', 'PreviewTime',
]:
    if name in BP.list_variables(bp):
        BP.set_variable_instance_editable(bp, name, False)

for graph_name in [
    'RadiantResolve', 'RadiantPlaceOrigin', 'RadiantComputeCoverage',
    'RadiantPrepareStatic', 'RadiantPrepareSkeletal', 'RadiantPrepare',
    'RadiantRestoreStatic', 'RadiantRestoreSkeletal', 'RadiantRestore',
    'RadiantSpawnStaticParticles', 'RadiantSpawnSkeletalParticles',
    'RadiantSpawnParticles', 'RadiantApplyFrame', 'RadiantReset',
    'RadiantPlay', 'RadiantFinish', 'RadiantUpdate', 'RadiantNoOp',
]:
    if not BP.get_graph(bp, graph_name):
        BP.add_function_graph(bp, graph_name)

BP.compile_blueprint(bp)
cdo = BP.get_default_object(bp)
defaults = {
    'RadiantFallbackMaterial': u.load_asset(ROOT + '/M_RadiantDissolve_Surface'),
    'RadiantSkeletalFallbackMaterial': u.load_asset(ROOT + '/M_RadiantDissolve_Gunner'),
    'RadiantAutoPlaceOrigin': True,
    'RadiantShowOriginMarker': True,
    'RadiantOriginHeightRatio': 0.22,
    'RadiantDuration': 2.4,
    'RadiantTravelDistance': 0.0,
    'RadiantNoiseStrength': 12.0,
    'RadiantHeatIntensity': 88.0,
    'RadiantEdgeWidth': 4.0,
    'RadiantDissolveStart': 0.24,
    'RadiantOutlineHoldStart': 0.66,
    'RadiantOutlineFadeStart': 0.80,
    'RadiantMeshGone': 0.95,
    'RadiantParticleRate': 4200.0,
    'RadiantParticleFrontWidth': 8.5,
    'RadiantPreviewEnabled': False,
    'RadiantPreviewTime': 0.0,
}
for name, value in defaults.items():
    cdo.set_editor_property(name, value)

BP.compile_blueprint(bp)
u.EditorAssetLibrary.save_loaded_asset(bp)
print('RADIANT_V2_SCHEMA_READY')
