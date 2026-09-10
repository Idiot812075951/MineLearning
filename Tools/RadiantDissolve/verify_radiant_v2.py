"""Structural verification report for the finalized radiant dissolve prototype."""
import json
import unreal as u
from editor_toolset.toolsets.actor import ActorTools as AT
from editor_toolset.toolsets.blueprint import BlueprintTools as BP

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
OUT = u.Paths.project_dir() + 'Saved/RadiantDissolve/verify_v2.json'
bp = u.load_asset(ROOT + '/BP_RadiantDissolve_Test')
BP.compile_blueprint(bp, warnings_as_errors=True)
cdo = BP.get_default_object(bp)

graph_names = [
    'RadiantResolve', 'RadiantPlaceOrigin', 'RadiantComputeCoverage',
    'RadiantPrepareStatic', 'RadiantPrepareSkeletal', 'RadiantPrepare',
    'RadiantRestoreStatic', 'RadiantRestoreSkeletal', 'RadiantRestore',
    'RadiantSpawnStaticParticles', 'RadiantSpawnSkeletalParticles',
    'RadiantSpawnParticles', 'RadiantApplyFrame', 'RadiantReset',
    'RadiantPlay', 'RadiantFinish', 'RadiantUpdate', 'RadiantNoOp',
    'RadiantEnablePreview', 'RadiantPreviewAt', 'Dissolve',
    'EventGraph',
]
graphs = {name: BP.read_graph_dsl(BP.get_graph(bp, name)) for name in graph_names}
suspicious = ['类|Aimat', 'Components|GeometryCache', 'TypedElementFramework|List|Reset']

controller = next(
    actor for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    if actor.get_class().get_name() == 'BP_RadiantDissolve_Test_C'
)
target_mesh = next(
    component for component in controller.get_components_by_class(u.StaticMeshComponent)
    if component.get_name().startswith('TargetMesh')
)
origin = next(
    component for component in controller.get_components_by_class(u.SceneComponent)
    if component.get_name().startswith('DissolveOrigin')
)
marker = next(
    component for component in controller.get_components_by_class(u.StaticMeshComponent)
    if component.get_name().startswith('OriginMarker')
)

names = [
    'RadiantDuration', 'RadiantTravelDistance', 'RadiantNoiseStrength',
    'RadiantHeatIntensity', 'RadiantEdgeWidth', 'RadiantDissolveStart',
    'RadiantOutlineHoldStart', 'RadiantOutlineFadeStart', 'RadiantMeshGone',
    'RadiantParticleRate', 'RadiantParticleFrontWidth',
    'RadiantParticleStart', 'RadiantParticleSpeed',
    'RadiantAutoPlaceOrigin', 'RadiantOriginHeightRatio',
    'RadiantShowOriginMarker', 'RadiantEffectiveRadius',
    'RadiantSkeletalFallbackMaterial',
    'RadiantAKMetalDissolveMaterial', 'RadiantAKPolymerDissolveMaterial',
    'RadiantAKMetalSourceMaterial', 'RadiantAKPolymerSourceMaterial',
]
report = {
    'blueprint_status': 'compiled_without_warnings',
    'empty_graphs': [name for name, code in graphs.items() if code.strip().endswith('())') and name != 'RadiantNoOp'],
    'suspicious_graph_tokens': {
        name: [token for token in suspicious if token in code]
        for name, code in graphs.items()
        if any(token in code for token in suspicious)
    },
    'event_graph': graphs['EventGraph'],
    'cdo_defaults': {
        name: str(cdo.get_editor_property(name))
        for name in names if name != 'RadiantEffectiveRadius'
    },
    'instance_values': {name: str(controller.get_editor_property(name)) for name in names},
    'target_static_mesh': target_mesh.static_mesh.get_path_name() if target_mesh.static_mesh else None,
    'target_actor': (
        controller.get_editor_property('RadiantTarget').get_path_name()
        if controller.get_editor_property('RadiantTarget') else None
    ),
    'target_relative_scale': [
        target_mesh.get_editor_property('relative_scale3d').x,
        target_mesh.get_editor_property('relative_scale3d').y,
        target_mesh.get_editor_property('relative_scale3d').z,
    ],
    'target_materials': [
        target_mesh.get_material(index).get_path_name() if target_mesh.get_material(index) else None
        for index in range(target_mesh.get_num_materials())
    ],
    'target_asset_materials': [
        slot.get_editor_property('material_interface').get_path_name()
        if slot.get_editor_property('material_interface') else None
        for slot in target_mesh.static_mesh.get_editor_property('static_materials')
    ],
    'target_skeletal_materials': [
        mesh.get_material(index).get_path_name() if mesh.get_material(index) else None
        for mesh in controller.get_editor_property('RadiantSkeletalMeshes')
        for index in range(mesh.get_num_materials())
    ],
    'origin_world': [origin.get_world_location().x, origin.get_world_location().y, origin.get_world_location().z],
    'marker_visible': marker.is_visible(),
    'marker_hidden_in_game': marker.get_editor_property('hidden_in_game'),
    'skeletal_sample_exists': u.EditorAssetLibrary.does_asset_exist(ROOT + '/BP_RadiantSkeletalTargetSample'),
    'skeletal_system_exists': u.EditorAssetLibrary.does_asset_exist(ROOT + '/NS_RadiantDissolve_Skeletal'),
    'gunner_surface_material_exists': u.EditorAssetLibrary.does_asset_exist(ROOT + '/M_RadiantDissolve_GunnerSurface'),
    'ak_metal_material_exists': u.EditorAssetLibrary.does_asset_exist(ROOT + '/M_RadiantDissolve_AKMetalSurface'),
    'ak_polymer_material_exists': u.EditorAssetLibrary.does_asset_exist(ROOT + '/M_RadiantDissolve_AKPolymerSurface'),
}
with open(OUT, 'w', encoding='utf-8') as handle:
    json.dump(report, handle, ensure_ascii=False, indent=2)
print('RADIANT_V2_VERIFIED', OUT)
