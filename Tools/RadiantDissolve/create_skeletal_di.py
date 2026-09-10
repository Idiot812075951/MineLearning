import pathlib
import unreal as u

system = u.load_asset('/Game/MineLearning/VFX/RadiantDissolve/NS_RadiantDissolve_Skeletal')
data_interface_class = u.load_class(None, '/Script/Niagara.NiagaraDataInterfaceSkeletalMesh')
data_interface = u.new_object(
    data_interface_class,
    outer=system,
    name='RadiantTargetSkeletalMeshDI',
)
path = data_interface.get_path_name()
out = pathlib.Path(u.Paths.project_dir()) / 'Tools/RadiantDissolve/skeletal_di_path.txt'
out.write_text(path, encoding='utf-8')
print('RADIANT_SKELETAL_DI', path)
