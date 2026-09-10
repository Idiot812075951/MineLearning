"""Reusable masked front; idle is explicitly intact regardless of noise or radius."""
import unreal as u
from editor_toolset.toolsets.material import MaterialTools as MT
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
E=u.EditorAssetLibrary
M=u.MaterialEditingLibrary

def node(owner, cls, x, y, **values):
    n=MT.add_expression(owner,cls.static_class(),x,y)
    for key,value in values.items():n.set_editor_property(key,value)
    return n

def scalar(owner,name,value,x,y):
    return node(owner,u.MaterialExpressionScalarParameter,x,y,parameter_name=name,default_value=value,group='Radiant Dissolve')

def custom(owner,description,code,inputs,typ,x,y):
    n=node(owner,u.MaterialExpressionCustom,x,y,description=description,code=code,output_type=typ)
    entries=[]
    for name in inputs:
        i=u.CustomInput();i.set_editor_property('input_name',name);entries.append(i)
    n.set_editor_property('inputs',entries)
    return n

def wire(a,b,pin,out=''):
    MT.connect_expressions(a,out,b,pin)

fn=E.load_asset(ROOT+'/MF_RadiantDissolve') or MT.create_function(ROOT,'MF_RadiantDissolve')
for n in MT.get_expressions(fn):MT.delete_expression(fn,n)
wp=node(fn,u.MaterialExpressionWorldPosition,-1100,0)
origin=node(fn,u.MaterialExpressionVectorParameter,-1100,180,parameter_name='DissolveOriginWS',default_value=u.LinearColor(0,0,0,0),group='Radiant Dissolve')
noise=custom(fn,'Contact anchored noise; amplitude fades near the origin',
    'float d=length(P-Origin); float3 q=P*.075; float n=sin(q.x+sin(q.z*1.31))*sin(q.y*1.17-q.z*.43)*.72+sin(q.x*3.7+q.z*2.1)*sin(q.y*3.1-q.x)*.28; return d+min(abs(Strength),d*.45)*n;',
    ['P','Origin','Strength'],u.CustomMaterialOutputType.CMOT_FLOAT1,-700,0)
wire(wp,noise,'P');wire(origin,noise,'Origin');wire(scalar(fn,'NoiseStrength',12,-1100,360),noise,'Strength')
masks=custom(fn,'Explicit idle and complete states. RGB: heat / edge / opacity',
    'if(Progress*Supported<=0) return float3(0,0,1); if(Progress>=1) return float3(0,0,0); float h=smoothstep(0,22,Heat-D); float rim=(1-smoothstep(0,max(Width,.1),D-Dissolve))*step(Dissolve,D)*step(0,Dissolve); return float3(h,rim,step(Dissolve,D));',
    ['D','Heat','Dissolve','Width','Progress','Supported'],u.CustomMaterialOutputType.CMOT_FLOAT3,-300,0)
wire(noise,masks,'D')
for index,(name,value,pin) in enumerate([('HeatRadius',-12,'Heat'),('DissolveRadius',-40,'Dissolve'),('DissolveEdgeWidth',2.5,'Width'),('DissolveProgress',0,'Progress'),('RadiantSupported',1,'Supported')]):
    wire(scalar(fn,name,value,-700,250+index*120),masks,pin)
out=node(fn,u.MaterialExpressionFunctionOutput,100,0,output_name='Masks',description='R=heat, G=bright edge, B=remaining opacity')
wire(masks,out,'')
MT.recompile(fn);E.save_loaded_asset(fn)

mat=E.load_asset(ROOT+'/M_RadiantDissolve_Test')
expressions=MT.get_expressions(mat)
surface=next(n for n in expressions if isinstance(n,u.MaterialExpressionCustom) and n.get_editor_property('description').startswith('Textured mineral'))
emission=next(n for n in expressions if isinstance(n,u.MaterialExpressionCustom) and n.get_editor_property('description').startswith('Controlled heat'))
mask=next(n for n in expressions if isinstance(n,u.MaterialExpressionComponentMask))
call=node(mat,u.MaterialExpressionMaterialFunctionCall,-420,0,material_function=fn)
wire(call,surface,'Masks');wire(call,emission,'Masks');wire(call,mask,'')
MT.delete_unused_expressions(mat);MT.recompile(mat);E.save_loaded_asset(mat)

# Neutral material example for non-ore targets; preserve their own authored look by
# integrating MF_RadiantDissolve into a copy of their actual master material.
generic=E.load_asset(ROOT+'/M_RadiantDissolve_Surface') or E.duplicate_asset(ROOT+'/M_RadiantDissolve_Test',ROOT+'/M_RadiantDissolve_Surface')
entries=MT.get_expressions(generic)
surface=next(n for n in entries if isinstance(n,u.MaterialExpressionCustom) and n.get_editor_property('description').startswith('Textured mineral'))
color=node(generic,u.MaterialExpressionVectorParameter,-400,-400,parameter_name='BaseColor',default_value=u.LinearColor(.15,.22,.29,1),group='Surface')
wire(color,surface,'Base','RGB')
MT.delete_unused_expressions(generic);MT.recompile(generic);E.save_loaded_asset(generic)

mote=E.load_asset(ROOT+'/M_RadiantMote')
for n in MT.get_expressions(mote):
    if isinstance(n,u.MaterialExpressionCustom) and 'cores' in n.get_editor_property('description'):
        n.set_editor_property('code','return float3(1,1,1)*150;')
        n.set_editor_property('description','White energy fragments')
MT.recompile(mote);E.save_loaded_asset(mote)
print('RADIANT_MATERIALS_UPGRADED')
