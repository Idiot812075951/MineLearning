import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as B
bp=u.load_asset('/Game/MineLearning/Characters/Guren/Blueprints/BPC_GurenQPresentation');g=B.get_graph(bp,'EventGraph')
def info(n):return B.get_node_infos([n])[0]
def pin(n,s,out=False):return next(p.pin_id for p in (info(n).output_pins if out else info(n).input_pins) if p.name==s)
ns=B.get_node_infos(B.find_nodes(g,''));reset=next(i.node for i in ns if i.type_id.endswith('|RadiantReset'));destroy=next(i.node for i in ns if i.type_id=='Actor|DestroyActor')
getfx=next(i.node for i in ns if i.type_id.endswith('|GetDissolveFX'))
if not any(i.type_id.endswith('|RadiantRestore') for i in ns):
 restore=B.create_node(g,'Class|BPRadiantDissolveTest|RadiantRestore',u.IntPoint(700,650))
 B.break_pins(pin(reset,'then',True),pin(destroy,'execute'))
 B.connect_pins(pin(reset,'then',True),pin(restore,'execute'));B.connect_pins(pin(getfx,'DissolveFX',True),pin(restore,'self'));B.connect_pins(pin(restore,'then',True),pin(destroy,'execute'))
B.compile_blueprint(bp);u.EditorAssetLibrary.save_loaded_asset(bp)
print('Q_MATERIAL_RESTORE_FIXED')
