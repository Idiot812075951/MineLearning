"""Inspect the actual automatic front camera during early dissolve, PIE only."""
import unreal as u,builtins
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
p=u.GameplayStatics.get_player_character(w,0)
q=p.get_component_by_class(u.GurenQSkillComponent)
u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings')).set_editor_property('bThrottleCPUWhenNotForeground',False)
q.try_cast()
state={'radiation_at':None}
def inspect(dt):
    if q.get_stage()==u.GurenQStage.RADIATION:
        now=u.GameplayStatics.get_real_time_seconds(w)
        if state['radiation_at'] is None:state['radiation_at']=now
        if now-state['radiation_at']>.6:
            u.GameplayStatics.set_game_paused(w,True)
            u.unregister_slate_post_tick_callback(builtins.q_front_preview)
            print('Q_FRONT_CAMERA_PAUSED')
builtins.q_front_preview=u.register_slate_post_tick_callback(inspect)
