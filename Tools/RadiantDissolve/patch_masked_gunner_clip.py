"""Force pixel discard in the Gunner masked adapter's surface shader."""
import unreal as u


path = '/Game/MineLearning/VFX/RadiantDissolve/M_RadiantDissolve_GunnerSurface'
material = u.load_asset(path)
editing = u.MaterialEditingLibrary
for expression in editing.get_material_expressions(material):
    if not isinstance(expression, u.MaterialExpressionCustom):
        continue
    if expression.get_editor_property('description') != 'Preserve Gunner surface while heating':
        continue
    expression.set_editor_property(
        'code',
        'float keep=max(Masks.b,step(.78,saturate(Fresnel))*saturate(Outline)); '
        'clip(keep-.20); '
        'float heat=saturate(Masks.x); '
        'float3 hot=lerp(Base,Base*float3(.92,.48,.44)+float3(.13,.002,.006),heat*.38); '
        'float shell=saturate(Outline)*smoothstep(.70,.96,saturate(Fresnel)); '
        'return lerp(hot,float3(.48,.002,.016),shell*.70);',
    )
    break
else:
    raise RuntimeError('Could not find Gunner surface custom expression')

editing.recompile_material(material)
u.EditorAssetLibrary.save_loaded_asset(material)
print('RADIANT_MASKED_GUNNER_CLIP_PATCHED')
