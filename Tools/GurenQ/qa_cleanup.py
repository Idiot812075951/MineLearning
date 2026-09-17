import unreal as u,gc,types
for obj in gc.get_objects():
 if isinstance(obj,types.FunctionType) and obj.__name__=='sample' and obj.__code__.co_filename.endswith('GurenQ/test_flow.py'):
  h=obj.__globals__.get('q_test_tick')
  if h:
   try:u.unregister_slate_post_tick_callback(h)
   except Exception:pass
settings=u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'))
settings.set_editor_property('bThrottleCPUWhenNotForeground',False)
print('Q_QA_CLEANED')
