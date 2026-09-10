"""Replace unsupported Scalar Blue Noise fades with a world-position hash."""
import unreal as u

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
editing = u.MaterialEditingLibrary


def custom_input(name):
    value = u.CustomInput()
    value.set_editor_property('input_name', name)
    return value


def connect(source, destination, input_name, output_name=''):
    if not editing.connect_material_expressions(source, output_name, destination, input_name):
        raise RuntimeError(
            f'Could not connect {source.get_name()} -> {destination.get_name()}.{input_name}'
        )


for asset_name in ['M_RadiantDissolve_Surface', 'M_RadiantDissolve_Test']:
    material = u.load_asset(ROOT + '/' + asset_name)
    expressions = editing.get_material_expressions(material)
    opacity = next(
        node for node in expressions
        if isinstance(node, u.MaterialExpressionScalarParameter)
        and str(node.get_editor_property('parameter_name')) == 'OpacityFade'
    )
    output = editing.get_material_property_input_node(
        material,
        u.MaterialProperty.MP_OPACITY_MASK,
    )
    if not isinstance(output, u.MaterialExpressionMultiply):
        raise RuntimeError(f'{asset_name} opacity output is not the expected multiply')

    existing = next(
        (
            node for node in expressions
            if isinstance(node, u.MaterialExpressionCustom)
            and node.get_editor_property('description') == 'World-position silhouette fade'
        ),
        None,
    )
    if existing:
        fade = existing
    else:
        position = editing.create_material_expression(
            material,
            u.MaterialExpressionWorldPosition,
            -290,
            900,
        )
        fade = editing.create_material_expression(
            material,
            u.MaterialExpressionCustom,
            -20,
            850,
        )
        fade.set_editor_property('description', 'World-position silhouette fade')
        fade.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT1)
        fade.set_editor_property(
            'inputs',
            [custom_input('Fade'), custom_input('Position')],
        )
        fade.set_editor_property(
            'code',
            'float n=frac(sin(dot(Position.xyz,float3(12.9898,78.233,37.719)))*43758.5453); '
            'return (Fade<=.001)?0:step(n,saturate(Fade));',
        )
        connect(opacity, fade, 'Fade')
        connect(position, fade, 'Position')
    connect(fade, output, 'B')
    editing.recompile_material(material)
    u.EditorAssetLibrary.save_loaded_asset(material)

print('RADIANT_BLUE_NOISE_FADES_REPLACED')
