"""Rebuild only the two dissolve materials with the held silhouette mask."""
from pathlib import Path

import unreal as u

source_path = Path(u.Paths.project_dir()) / 'Tools' / 'RadiantDissolve' / 'upgrade_v2_core.py'
source = source_path.read_text(encoding='utf-8')
definitions = source.split('\n\nore_texture =', 1)[0]
namespace = {}
exec(compile(definitions, str(source_path), 'exec'), namespace)

root = namespace['ROOT']
assets = namespace['E']
rebuild = namespace['rebuild_dissolve_material']
ore_texture = assets.load_asset('/Game/MineLearning/Mining/Ores/Iron/Textures/T_Ore_Iron_100_BaseColor')
rebuild(assets.load_asset(root + '/M_RadiantDissolve_Test'), ore_texture)
rebuild(assets.load_asset(root + '/M_RadiantDissolve_Surface'))
print('RADIANT_HELD_OUTLINE_REBUILT')
