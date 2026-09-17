"""Targeted final-stage revision. Run with PIE stopped, after baseline inspection."""
import unreal as u, json, shutil
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.material import MaterialTools as MT
O=Path(__file__).parent
PROJECT=O.parents[2]
R='/Game/MineLearning/VFX/RadiantDissolve/'
E=u.EditorAssetLibrary
b=u.load_asset(R+'BP_RadiantDissolve_Test')
names=['BP_RadiantDissolve_Test','M_RadiantDissolve_Surface','M_RadiantDissolve_GunnerSurface','M_RadiantDissolve_AKMetalSurface','M_RadiantDissolve_AKPolymerSurface','M_RadiantDissolve_AKMagazineSurface','NS_RadiantDissolve_Test','NS_RadiantDissolve_Skeletal']
backup=PROJECT/'Saved/RadiantFinalScatter/Before'
backup.mkdir(parents=True,exist_ok=True)
for n in names:
    a=u.load_asset(R+n)
    u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(a)
    # Save the in-memory source first, so the rollback includes user tuning.
    if not (backup/(n+'.uasset')).exists():
        E.save_loaded_asset(a)
        shutil.copy2(PROJECT/('Content/MineLearning/VFX/RadiantDissolve/'+n+'.uasset'),backup/(n+'.uasset'))

settings={'RadiantSweepEnd':.30,'RadiantDissolveStart':.34,'RadiantMeshGone':.52,
          'RadiantParticleStart':.34,'RadiantParticleEnd':.57,'RadiantParticleInwardDepth':10.,
          'RadiantParticleRate':18000.,'RadiantParticleSpeed':85.,'RadiantHeatIntensity':65.}
for n in ['RadiantSweepEnd','RadiantParticleEnd','RadiantParticleInwardDepth']:
    if n not in B.list_variables(b):B.add_variable(b,n,'float')
for n in settings:
    B.set_variable_instance_editable(b,n,True)
    B.set_variable_category(b,n,'Radiant Dissolve')

def clamp(value):return f'(工具|Select (工具|Select {value} 1.0 (> {value} 1.0)) 0.0 (< {value} 0.0))'
graph='''(fn RadiantApplyFrame ()
 (bind duration (工具|Select .1 (Variables|RadiantDissolve|GetRadiantDuration) (> (Variables|RadiantDissolve|GetRadiantDuration) .1)))
 (bind rawT (工具|Select (/ (Variables|RadiantInternal|GetRadiantElapsed) duration) (Variables|RadiantInternal|GetRadiantPreviewTime) (Variables|RadiantInternal|GetRadiantPreviewEnabled)))
 (bind t CLAMP_T)
 (bind reach (工具|Select (Variables|RadiantInternal|GetRadiantEffectiveRadius) (Variables|RadiantInternal|GetRadiantTravelDistance) (> (Variables|RadiantInternal|GetRadiantTravelDistance) 0)))
 (bind maxNoise (* reach .22))
 (bind noise (工具|Select (Variables|RadiantDissolve|GetRadiantNoiseStrength) maxNoise (> (Variables|RadiantDissolve|GetRadiantNoiseStrength) maxNoise)))
 (bind sweepEnd (工具|Select .01 (Variables|RadiantDissolve|GetRadiantSweepEnd) (> (Variables|RadiantDissolve|GetRadiantSweepEnd) .01)))
 (bind sweepRaw (/ t sweepEnd))
 (bind sweep CLAMP_SWEEP)
 (bind start (Variables|RadiantDissolve|GetRadiantDissolveStart))
 (bind gone (Variables|RadiantDissolve|GetRadiantMeshGone))
 (bind erosionDuration (工具|Select .01 (- gone start) (> (- gone start) .01)))
 (bind erosionRaw (/ (- t start) erosionDuration))
 (bind erosion CLAMP_EROSION)
 (bind padding (+ noise 30))
 (bind heat (- (* sweep (+ (+ reach padding) 30)) padding))
 (bind dissolve (- (* erosion (+ reach (* padding 2))) padding))
 (bind opacity (工具|Select 1.0 0.0 (>= t gone)))
 (bind count (+ (Utilities|Array|Length (Variables|RadiantInternal|GetRadiantStaticMeshes)) (Utilities|Array|Length (Variables|RadiantInternal|GetRadiantSkeletalMeshes))))
 (bind emitStarted (工具|Select 0.0 1.0 (>= t (Variables|RadiantDissolve|GetRadiantParticleStart))))
 (bind emitWindow (工具|Select emitStarted 0.0 (>= t (Variables|RadiantDissolve|GetRadiantParticleEnd))))
 (bind rate (/ (* emitWindow (Variables|RadiantDissolve|GetRadiantParticleRate)) (工具|Select 1 count (> count 0))))
 (bind origin (Transformation|GetWorldLocation (Variables|Default|GetDissolveOrigin)))
'''.replace('CLAMP_T',clamp('rawT')).replace('CLAMP_SWEEP',clamp('sweepRaw')).replace('CLAMP_EROSION',clamp('erosionRaw'))
for kind in ['Static','Skeletal']:
    graph+=f' (for mesh (Variables|RadiantInternal|GetRadiant{kind}Meshes)\n'
    graph+='  (Rendering|Material|SetVectorParameterValueOnMaterials mesh "DissolveOriginWS" origin)\n'
    for param,val in [('DissolveProgress','t'),('HeatRadius','heat'),('DissolveRadius','dissolve'),('NoiseStrength','noise'),('HeatIntensity','(Variables|RadiantDissolve|GetRadiantHeatIntensity)'),('DissolveEdgeWidth','(Variables|RadiantDissolve|GetRadiantEdgeWidth)'),('OutlineAmount','0.0'),('OpacityFade','opacity')]:
        graph+=f'  (Rendering|Material|SetScalarParameterValueOnMaterials mesh "{param}" {val})\n'
    graph+=' )\n'
graph+=' (for fx (Variables|RadiantInternal|GetRadiantActiveParticles)\n'
graph+='  (Niagara|SetNiagaraVariable(Position) fx "User.DissolveOriginWS" origin)\n'
for param,val in [('DissolveRadius','dissolve'),('SpawnRate','rate'),('ParticleSpeed','(Variables|RadiantDissolve|GetRadiantParticleSpeed)'),('InwardDepth','(Variables|RadiantDissolve|GetRadiantParticleInwardDepth)'),('EffectDuration','duration')]:
    graph+=f'  (Niagara|SetNiagaraVariable(Float) fx "User.{param}" {val})\n'
graph+=' ))'
# The editor is running in English. Use the language-independent select form.
import re
graph=graph.replace('SetNiagaraVariable(Position)','SetNiagaraVariable_POSITION').replace('SetNiagaraVariable(Float)','SetNiagaraVariable_FLOAT')
tokens=re.findall(r'"[^"\\]*(?:\\.[^"\\]*)*"|[^\s()]+|[()]',graph)
it=iter(tokens)
def parse(first):
    if first!='(':return first
    a=[]
    for token in it:
        if token==')':break
        a.append(parse(token))
    if a and a[0]=='工具|Select':a=['select',a[3],a[2],a[1]]
    return a
def render(a):return '('+' '.join(render(v) for v in a)+')' if isinstance(a,list) else a
graph=render(parse(next(it))).replace('SetNiagaraVariable_POSITION','SetNiagaraVariable(Position)').replace('SetNiagaraVariable_FLOAT','SetNiagaraVariable(Float)')
(O/'RadiantApplyFrame.dsl').write_text(graph,encoding='utf8')
B.write_graph_dsl(B.get_graph(b,'RadiantApplyFrame'),graph)
for kind,system,di,setter in [('Static','NS_RadiantDissolve_Test','User.TargetMesh','SetNiagaraStaticMeshComponent'),('Skeletal','NS_RadiantDissolve_Skeletal','User.TargetSkeletalMesh','SetNiagaraSkeletalMeshComponent')]:
    B.write_graph_dsl(B.get_graph(b,'RadiantSpawn'+kind+'Particles'),f'''(fn RadiantSpawn{kind}Particles ()
      (for mesh (Variables|RadiantInternal|GetRadiant{kind}Meshes)
       (bind fx (Niagara|SpawnSystemAttached :SystemTemplate "{R}{system}.{system}" :AttachToComponent mesh :bAutoDestroy false :bAutoActivate false :bPreCullCheck false))
       (Niagara|{setter} fx "{di}" mesh)
       (bind (center extent radius) (Collision|GetComponentBounds mesh))
       (Niagara|SetNiagaraVariable(Position) fx "User.ScatterCenterWS" center)
       (Utilities|Array|Add (Variables|RadiantInternal|GetRadiantActiveParticles) fx)))''')
B.compile_blueprint(b,warnings_as_errors=True)
c=u.get_default_object(b.generated_class())
for n,v in settings.items():c.set_editor_property(n,v)
E.save_loaded_asset(b)

for name in names[1:6]:
    mat=u.load_asset(R+name)
    for x in MT.get_expressions(mat):
        if not isinstance(x,u.MaterialExpressionCustom):continue
        inputs={str(i.get_editor_property('input_name')) for i in x.get_editor_property('inputs')}
        if {'Masks','Intensity'}<=inputs:
            x.set_editor_property('code','return max(Intensity,0)*float3(1.0,.94,.86)*(pow(saturate(Masks.x),1.4)*.52 + saturate(Masks.y)*.12);')
            x.set_editor_property('description','Full surface energy sweep; no Fresnel shell')
        elif {'Remaining','Outline','Fresnel'}<=inputs:
            x.set_editor_property('code','return Remaining;')
            x.set_editor_property('description','No residual outline after surface conversion')
        elif {'Base','Masks','Outline','Fresnel'}<=inputs:
            x.set_editor_property('code','return lerp(Base,float3(.78,.75,.70),saturate(Masks.x)*.55);')
            x.set_editor_property('description','Source material to bright surface')
    u.MaterialEditingLibrary.recompile_material(mat);E.save_loaded_asset(mat)
for n in names[-2:]:E.save_loaded_asset(u.load_asset(R+n))
print('FINAL_SCATTER_SURFACE_APPLIED',settings)
