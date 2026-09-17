import unreal as u,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A
R='/Game/MineLearning/Characters/Guren';E=u.EditorAssetLibrary;O=Path(u.Paths.project_saved_dir())/'GurenQ'
def info(n):return B.get_node_infos([n])[0]
def pin(n,name,out=False):return next(p.pin_id for p in (info(n).output_pins if out else info(n).input_pins) if p.name==name)
def link(a,ao,b,bi):B.connect_pins(pin(a,ao,True),pin(b,bi))
pb=E.load_asset(R+'/Blueprints/BPC_GurenQPresentation');g=B.get_graph(pb,'EventGraph');ns=B.get_node_infos(B.find_nodes(g,''))
spawn=next(i.node for i in ns if i.node.get_class().get_name()=='K2Node_SpawnActorFromClass')
mt=B.create_node(g,'Math|Transform|MakeTransform',u.IntPoint(0,400));B.set_pin_value(pin(mt,'Location'),'0,0,-10000');link(mt,'ReturnValue',spawn,'SpawnTransform')
valid=next(i.node for i in ns if i.type_id=='Utilities|IsValid' and any(p.connected_pins for p in i.input_pins if p.type_id=='Exec'))
getfx=B.create_node(g,'Variables|Default|GetDissolveFX',u.IntPoint(300,700))
reset=B.create_node(g,'Class|BPRadiantDissolveTest|RadiantReset',u.IntPoint(550,650));destroy=B.create_node(g,'Actor|DestroyActor',u.IntPoint(800,650))
link(valid,'Is Valid',reset,'execute');link(getfx,'DissolveFX',reset,'self');link(reset,'then',destroy,'execute');link(getfx,'DissolveFX',destroy,'self')
B.compile_blueprint(pb);E.save_loaded_asset(pb);(O/'QPresentation_final.dsl').write_text(B.read_graph_dsl(g),encoding='utf8')
player=E.load_asset(R+'/Blueprints/BP_GurenRetargetTest');g=B.get_graph(player,'EventGraph');ns=B.get_node_infos(B.find_nodes(g,''))
if E.get_metadata_tag(player,'QAttackGate')!='1':
 ev=next(i.node for i in ns if i.type_id=='AddEvent|Custom|TryAttack');ep=next(p for p in info(ev).output_pins if p.name=='then');old=ep.connected_pins[0]
 branch=B.create_node(g,'Utilities|FlowControl|Branch',u.IntPoint(1000,1500));qget=B.create_node(g,'Variables|QSkill|GetQSkill',u.IntPoint(600,1650));active=B.create_node(g,'QSkill|IsQActive',u.IntPoint(800,1650))
 link(qget,'QSkill',active,'self');link(active,'ReturnValue',branch,'Condition');B.break_pins(ep.pin_id,old);link(ev,'then',branch,'execute');B.connect_pins(pin(branch,'else',True),old)
 E.set_metadata_tag(player,'QAttackGate','1')
B.compile_blueprint(player);E.save_loaded_asset(player)
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('Q_GRAPHS_COMPILED')
