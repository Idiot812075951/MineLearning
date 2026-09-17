# 红莲圣天八极式 — 当前源资产 R19

唯一 Blender 源文件：`ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R19_Refinement.blend`。
UE 正式网格：`/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren`；测试关卡：`/Game/MineLearning/Maps/L_Guren_Retarget_Test`。

保留红甲、橙金胸部、青绿镜片、深灰内构、银色辐射臂、红核金爪及粉色推进翼。动画设定决定比例轮廓，商品近景补充连接结构；参考入口见 [参考图库](../ArtSource/Characters/GurenSeitenHakkyoShiki/References/REFERENCE_INDEX.md)。

R19 已精简腿部、背包和翼根叠层，修整头盔与指节曲面。保护当前生产骨架、Rest Pose、骨长、父级、绑定、UV、材质槽、右手抓取挂点及已有 Action。文件内的编辑辅助结构和动作由后续明确任务决定，本次只清理外部文件。

当前运行时支持地面移动和攻击、持续飞行、能量恢复，以及 Q 突进抓取和辐射处决。Space 长按进入飞行并上升，水平输入维持飞行；进入 Flying 有 0.5 秒缓冲，全部松开后保留 1 秒飞行姿态，再以 0.35 秒混入下落。地面保留原翼姿，耗尽能量优先下落。

- [R19 交付说明](../Docs/Guren_Refinement_R19_Delivery.md)：模型改动、UE 交付与既有验证结果。
- [Q 配置说明](../Docs/Guren_Q_Configuration.md)：目标、动画通知、镜头、取消恢复。
- [当前溶解效果](../Docs/VFX/RadiantDissolve_FinalScatter.md)：共享特效配置。

同一红莲资产只保留此 R19 `.blend` 和上述红莲说明、参考索引共四份 Markdown；旧版、阶段 Brief、导出副本、脚本、预览和回退文件不再保留。后续修改以用户当前任务为准。
