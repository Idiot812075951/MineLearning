"""Make the pre-dissolve red silhouette readable and guarantee a clean final fade."""
import unreal as u

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
editing = u.MaterialEditingLibrary


def polish(path):
    material = u.load_asset(path)
    if not material:
        raise RuntimeError(f'Missing material: {path}')

    for node in editing.get_material_expressions(material):
        if isinstance(node, u.MaterialExpressionFresnel):
            node.set_editor_property('exponent', 1.35)
        if not isinstance(node, u.MaterialExpressionCustom):
            continue
        description = node.get_editor_property('description')
        if description == 'Preserve Gunner surface while heating':
            node.set_editor_property(
                'code',
                'float heat=saturate(Masks.x); '
                'float3 hot=lerp(Base,Base*float3(.90,.42,.40)+float3(.16,.002,.006),heat*.42); '
                'float shell=saturate(Outline)*smoothstep(.34,.82,saturate(Fresnel)); '
                'return lerp(hot,float3(.44,.002,.012),shell*.58);',
            )
        elif description == 'Gunner heat edge and early red silhouette':
            node.set_editor_property(
                'code',
                'float front=pow(saturate(Masks.y),1.45); '
                'float shell=saturate(Outline)*smoothstep(.30,.78,saturate(Fresnel)); '
                'return Base*.85+Intensity*(pow(saturate(Masks.x),1.45)*float3(1,.008,.018)*.055 '
                '+front*float3(1,.42,.58)*.30+shell*float3(1,.018,.055)*.060);',
            )
        elif description == 'Heat surface plus held cartoon silhouette':
            node.set_editor_property(
                'code',
                'float3 heated=lerp(Base,float3(.25,.004,.012),Masks.x*.9); '
                'float shell=saturate(Outline)*smoothstep(.34,.82,saturate(Fresnel)); '
                'float hot=saturate(Masks.y); '
                'return lerp(heated,float3(.44,.002,.012),shell*.72)'
                '+hot*float3(.55,.018,.035);',
            )
        elif description == 'Red heat, white-pink front and lingering silhouette':
            node.set_editor_property(
                'code',
                'float front=pow(saturate(Masks.y),1.45); '
                'float shell=saturate(Outline)*smoothstep(.30,.78,saturate(Fresnel)); '
                'return Intensity*(pow(saturate(Masks.x),1.45)*float3(1,.008,.018)*.34 '
                '+front*float3(1,.42,.58)*2.15+shell*.90*float3(1,.025,.07));',
            )
        elif description in ('Blue-noise Gunner silhouette fade', 'Blue-noise silhouette fade'):
            # Scalar Blue Noise is signed. Remap it so Fade=0 always removes everything.
            node.set_editor_property(
                'code',
                'return (Fade<=.001)?0:step(saturate(Noise*.5+.5),saturate(Fade));',
            )

    editing.recompile_material(material)
    u.EditorAssetLibrary.save_loaded_asset(material)


for asset_path in [
    ROOT + '/M_RadiantDissolve_Gunner',
    ROOT + '/M_RadiantDissolve_Surface',
    ROOT + '/M_RadiantDissolve_Test',
]:
    polish(asset_path)

print('RADIANT_SEQUENCE_MATERIALS_POLISHED')
