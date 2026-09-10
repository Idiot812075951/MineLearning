"""Open the Gunner adapter in the Material Editor for diagnostics."""
import unreal as u

asset = u.load_asset('/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_Gunner')
u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([asset])
print('RADIANT_GUNNER_ADAPTER_OPENED')
