# ThiefHound / 偷矿兽 — 当前源资产 P06A

唯一源文件：`ArtSource/ThiefHound/ThiefHound_P06A_AnimationRecovery.blend`。
设计参考：`ArtSource/ThiefHound/References/ThiefHound_Concept_P06.png`。

用途为低伏、敏捷的四足敌对偷矿单位：寻找 Pickup、携带赃物逃跑，货物由 UE 动态挂载及掉落。当前归档为 Blender 模型和动画，不代表已经实现 UE 敌人玩法。

外观保留犬科长头、独立机械下颚、四条机械腿、红色眼灯、向后头鳍与独立开放货架。采用暗工业红身份壳、Gunmetal、Tool Silver、Dark Rubber 和少量橙色卡扣；不加入固定矿石、武器、钳子或封闭背包。

尺度目标为长 130–160 cm、高 65–80 cm、宽 50–65 cm，货架有效区域约 80 × 60 cm。地面根节点为原点参考，+X 向前、+Z 向上。

保护 `RIG_ThiefHound`、16 根骨骼的名称/父级/Rest Pose、刚性 Vertex Group、Armature Modifier 及 Cargo/Guide 点。保留 `ThiefHound_Idle / Walk / Run / Hit / Death` 五套 Action、关键帧、Fake User 与正确 Action Slot 归属。

辅助点包括 `CargoDisplayPoint`、`HeadPoint`、`BodyCenterPoint`、`VFX_HitPoint`、`VFX_DeathPoint`。正式模型货架保持空置，测试货物不得并入导出。

P06A 解决动作归属与恢复问题；后续不得将文件清理扩大为骨架、外观或动画重做。仅保留这份当前说明和最新 `.blend`，旧阶段及检查点已移除。
