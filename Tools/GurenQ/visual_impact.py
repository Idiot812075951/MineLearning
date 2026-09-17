"""Pause during a dash to inspect afterimage silhouette; PIE only."""
import unreal as u, builtins
w = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
p = u.GameplayStatics.get_player_character(w, 0)
pc = u.GameplayStatics.get_player_controller(w, 0)
q = p.get_component_by_class(u.GurenQSkillComponent)
pc.set_control_rotation(u.Rotator(pitch=-14, yaw=-65))
p.get_component_by_class(u.SpringArmComponent).set_editor_property('target_arm_length', 1100)
u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings')).set_editor_property('bThrottleCPUWhenNotForeground', False)
q.try_cast()
start = u.GameplayStatics.get_time_seconds(w)
def pause_dash(dt):
    if u.GameplayStatics.get_time_seconds(w) - start > .43:
        u.GameplayStatics.set_game_paused(w, True)
        u.unregister_slate_post_tick_callback(builtins.q_preview_tick)
        print('Q_DASH_PREVIEW_PAUSED')
builtins.q_preview_tick = u.register_slate_post_tick_callback(pause_dash)
