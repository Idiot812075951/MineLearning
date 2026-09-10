import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
bp = u.load_asset(ROOT + '/BP_RadiantDissolve_Test')
if 'OriginReference' not in BP.list_variables(bp):
    BP.add_object_variable(bp, 'OriginReference', u.PrimitiveComponent.static_class())
BP.compile_blueprint(bp)

BP.write_graph_dsl(BP.get_graph(bp, 'ResolveTargets'), '''(fn ResolveTargets ()
 (Utilities|Array|Clear (变量|Default|GetBoundMeshes))
 (Utilities|Array|Clear (变量|Default|GetBoundSkeletalMeshes))
 (Utilities|IsValid (变量|Target|GetTargetActor)
  (:"Is Valid"
   (Rendering|SetVisibility :self (变量|Default|GetTargetMesh) :bNewVisibility false)
   (for component (Actor|GetComponentsByClass :self (变量|Target|GetTargetActor) :ComponentClass "/Script/Engine.StaticMeshComponent")
    (bind mesh (工具|Casting|CastToStaticMeshComponent :Object component)
     (:then
      (Utilities|Array|Add (变量|Default|GetBoundMeshes) mesh)
      (变量|Default|SetOriginReference mesh))
     (:CastFailed)))
   (for component (Actor|GetComponentsByClass :self (变量|Target|GetTargetActor) :ComponentClass "/Script/Engine.SkeletalMeshComponent")
    (bind mesh (工具|Casting|CastToSkeletalMeshComponent :Object component)
     (:then
      (Utilities|Array|Add (变量|Default|GetBoundSkeletalMeshes) mesh)
      (变量|Default|SetOriginReference mesh))
     (:CastFailed))))
  (:"Is Not Valid"
   (Rendering|SetVisibility :self (变量|Default|GetTargetMesh) :bNewVisibility true)
   (Utilities|Array|Add (变量|Default|GetBoundMeshes) (变量|Default|GetTargetMesh))
   (变量|Default|SetOriginReference (变量|Default|GetTargetMesh)))))''')

BP.write_graph_dsl(BP.get_graph(bp, 'UpdateAutoOrigin'), '''(fn UpdateAutoOrigin ()
 (工具|流程控制|Branch (变量|Target|GetAutoPlaceOrigin)
  (:then
   (bind (meshCenter meshExtent meshRadius) (Collision|GetComponentBounds (变量|Default|GetOriginReference)))
   (bind meshHeight (* (Math|Vector|VectorLength meshExtent) (变量|Target|GetOriginHeightRatio)))
   (Transformation|SetWorldLocation :self (变量|Default|GetDissolveOrigin) :NewLocation (+ meshCenter (Math|Vector|MakeVector :X 0 :Y 0 :Z meshHeight)) :bSweep false :bTeleport true))
  (:else)))''')

BP.write_graph_dsl(BP.get_graph(bp, 'Initialize'), '''(fn Initialize ()
 (CallFunction|ResolveTargets)
 (CallFunction|UpdateAutoOrigin)
 (变量|Default|SetEffectiveRadius 0)
 (bind origin (Transformation|GetWorldLocation :self (变量|Default|GetDissolveOrigin)))
 (for mesh (变量|Default|GetBoundMeshes)
  (bind (center extent radius) (Collision|GetComponentBounds mesh))
  (bind reach (+ (+ (Math|Vector|Distance(Vector) center origin) (Math|Vector|VectorLength extent)) 2))
  (变量|Default|SetEffectiveRadius (工具|Select (变量|Default|GetEffectiveRadius) reach (> reach (变量|Default|GetEffectiveRadius)))))
 (for mesh (变量|Default|GetBoundSkeletalMeshes)
  (bind (center extent radius) (Collision|GetComponentBounds mesh))
  (bind reach (+ (+ (Math|Vector|Distance(Vector) center origin) (Math|Vector|VectorLength extent)) 2))
  (变量|Default|SetEffectiveRadius (工具|Select (变量|Default|GetEffectiveRadius) reach (> reach (变量|Default|GetEffectiveRadius))))))''')

BP.compile_blueprint(bp)
u.EditorAssetLibrary.save_loaded_asset(bp)
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label() == 'RadiantDissolve_Test':
        actor.call_method('Reset')
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
print('RADIANT_ORIGIN_GRAPHS_REPAIRED')
