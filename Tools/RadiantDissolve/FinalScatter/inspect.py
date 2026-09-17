import unreal as u, json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.material import MaterialTools as M
O=Path(__file__).parent
R='/Game/MineLearning/VFX/RadiantDissolve/'
b=u.load_asset(R+'BP_RadiantDissolve_Test'); c=u.get_default_object(b.generated_class())
r={'defaults':{str(n):str(c.get_editor_property(n)) for n in B.list_variables(b)},'graphs':{}}
for n in ['RadiantApplyFrame','RadiantSpawnSkeletalParticles','RadiantFinish','RadiantUpdate']:
    r['graphs'][n]=str(B.read_graph_dsl(B.get_graph(b,n)))
p=u.load_asset('/Game/MineLearning/Characters/Guren/Blueprints/BPC_GurenQPresentation')
r['presentation']=str(B.read_graph_dsl(B.get_graph(p,'EventGraph')))
r['materials']={}
for name in ['M_RadiantDissolve_Surface','MF_RadiantDissolve']:
    a=u.load_asset(R+name)
    r['materials'][name]=[]
    for x in M.get_expressions(a):
        if isinstance(x,u.MaterialExpressionCustom):
            r['materials'][name].append({'path':x.get_path_name(),'description':x.get_editor_property('description'),'code':x.get_editor_property('code'),'inputs':[str(i.get_editor_property('input_name')) for i in x.get_editor_property('inputs')]})
(O/'baseline.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf8')
print('FINAL_SCATTER_INSPECT_DONE')
