import unreal as u,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
R='/Game/MineLearning/Characters/Guren'
bp=u.load_asset(R+'/Blueprints/BPC_GurenQPresentation');c=u.get_default_object(bp.generated_class())
names=['dash_montage','grab_montage','afterimage_material','impact_strength','close_up_fov','contact_time_scale','contact_slow_motion_duration','dissolve_shake_duration','use_execution_camera','execution_camera_distance','execution_camera_yaw_offset','execution_camera_pitch','execution_camera_height','execution_camera_blend_in','execution_camera_blend_out']
result={'presentation':{n:str(c.get_editor_property(n)) for n in names}}
fx=u.load_asset('/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test');fd=u.get_default_object(fx.generated_class())
result['dissolve']={n:str(fd.get_editor_property(n)) for n in ['RadiantDuration','RadiantShowOriginMarker','RadiantNoiseStrength','RadiantHeatIntensity','RadiantEdgeWidth','RadiantParticleRate','RadiantParticleStart','RadiantParticleSpeed']}
result['presentation_graph']=B.read_graph_dsl(B.get_graph(bp,'EventGraph'))
(Path(u.Paths.project_saved_dir())/'GurenQ/configuration.json').write_text(json.dumps(result,indent=2),encoding='utf8')
print('Q_CONFIGURATION_READ')
