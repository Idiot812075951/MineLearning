import unreal as u,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
R='/Game/MineLearning/VFX/RadiantDissolve/'
b=u.load_asset(R+'BP_RadiantDissolve_Test')
B.compile_blueprint(b,warnings_as_errors=True)
for n in ['BP_RadiantDissolve_Test','M_RadiantDissolve_Surface','M_RadiantDissolve_GunnerSurface','M_RadiantDissolve_AKMetalSurface','M_RadiantDissolve_AKPolymerSurface','M_RadiantDissolve_AKMagazineSurface','NS_RadiantDissolve_Test','NS_RadiantDissolve_Skeletal']:
    u.EditorAssetLibrary.save_loaded_asset(u.load_asset(R+n))
c=u.get_default_object(b.generated_class())
names=['RadiantSweepEnd','RadiantDissolveStart','RadiantMeshGone','RadiantParticleStart','RadiantParticleEnd','RadiantParticleInwardDepth','RadiantParticleRate','RadiantParticleSpeed','RadiantHeatIntensity','RadiantNoiseStrength','RadiantEdgeWidth','RadiantDuration']
(Path(__file__).parent/'final_defaults.json').write_text(json.dumps({n:c.get_editor_property(n) for n in names},indent=2))
u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([b])
print('FINAL_SCATTER_SAVED')
