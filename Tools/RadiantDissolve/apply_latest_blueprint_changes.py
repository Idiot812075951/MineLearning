"""Apply the current source-parameter and phase-handoff Blueprint edits in order."""
from pathlib import Path
import runpy


root = Path(__file__).resolve().parent
for script in (
    # Create the small late-stage handoff function before RadiantApplyFrame calls it.
    'add_gunner_fade_stage.py',
    'revise_order_and_gunner_surface.py',
    # Keep this last: it owns source-instance parameter preservation for both
    # static and skeletal material preparation.
    'preserve_source_material_parameters.py',
    'add_ak_static_material_adapters.py',
    'repair_runtime_flow_and_tuning.py',
):
    runpy.run_path(str(root / script), run_name='__main__')
print('RADIANT_LATEST_BLUEPRINT_CHANGES_APPLIED')
