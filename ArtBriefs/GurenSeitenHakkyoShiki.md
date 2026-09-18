# 红莲圣天八极式 — 当前源资产 R19

唯一 Blender 源文件：`ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R19_Refinement.blend`。
2026-09-17 Q 扩展范围：统一可抓取组件、TAB 选目标、爪子指示器、按目标尺寸放大和镜头构图。图标的姿势源为 UE `AN_Guren_Q_GrabHeadIK` 的 GrabContact 帧，透明源图为 `ArtSource/UI/Guren/T_GurenGrabClaw.png`。本轮不改 Blender、骨架、Rest Pose、UV 或现有动画关键帧。
2026-09-18 首版范围（已交付，V3 范围见下）：仅优化 HUD 图标为大掌心、短五指，选中显示原图、候选降低透明度；新增挂在现有 `socket_palm_fx` 的红色波纹、细粒与少量短电弧。静态角色源仍为 R19；图标源为正式 PNG，特效源为 UE Niagara/材质。保护角色外形、骨架、Rest Pose、绑定、动画和共享溶解效果。
UE 正式网格：`/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren`；测试关卡：`/Game/MineLearning/Maps/L_Guren_Retarget_Test`。
2026-09-18 掌心修正：三层 Sprite 显式应用 Owner Scale；电弧改为可辨识的短分叉折线；呼吸与电弧跳动独立配置，降低默认频率。仅修改正式 Niagara/材质及其验证，不改角色源模型与动画。
2026-09-18 V3 当前范围：依据用户提供的《Guren_RadiantWave_VFX_V3_Construction》优化正式 UE 掌心 Niagara、材质与 Q 表现组件。视觉中心为右掌辐射器与接触点：小型白热核、破碎薄波、短分叉弧、透明空气折射、局部补光；统一强度随真实动画阶段变化。允许调整现有溶解的升温颜色及节奏，保留目标前期轮廓和原有功能。继续保护 R19 模型、骨架、动画、Q 玩法及翼部效果；验收覆盖不同体型、多视角、热浪开关、取消和重复释放。

2026-09-18 材质保真修复：抓取辐射保留目标原着色器，统一使用独立临时 MID 和共享材质函数，不再按目标或材质名称替换通用表面。源为现有 UE 目标材质图及实例；保护原纹理、配色、表面通道与运行时业务参数，不改模型、Rig、UV 或动画。六类目标及重复抓取已完成实机回归，结果见 Q 配置说明。

2026-09-18 后续修复范围：补齐小白人 `M_Mannequin`、矿石各采矿阶段及受损材质的共享辐射函数接入，并核对实际遮罩渲染模式。继续保护原材质外观和采矿阶段业务，不加入按目标类别处理的运行时分支；验收必须检查表面红热、逐步裁切和取消恢复，不能仅检查材质对象保留。

保留红甲、橙金胸部、青绿镜片、深灰内构、银色辐射臂、红核金爪及粉色推进翼。动画设定决定比例轮廓，商品近景补充连接结构；参考入口见 [参考图库](../ArtSource/Characters/GurenSeitenHakkyoShiki/References/REFERENCE_INDEX.md)。

R19 已精简腿部、背包和翼根叠层，修整头盔与指节曲面。保护当前生产骨架、Rest Pose、骨长、父级、绑定、UV、材质槽、右手抓取挂点及已有 Action。文件内的编辑辅助结构和动作由后续明确任务决定，本次只清理外部文件。

当前运行时支持地面移动和攻击、持续飞行、能量恢复，以及 Q 突进抓取和辐射处决。Space 长按进入飞行并上升，水平输入维持飞行；进入 Flying 有 0.5 秒缓冲，全部松开后保留 1 秒飞行姿态，再以 0.35 秒混入下落。地面保留原翼姿，耗尽能量优先下落。

- [R19 交付说明](../Docs/Guren_Refinement_R19_Delivery.md)：模型改动、UE 交付与既有验证结果。
- [Q 配置说明](../Docs/Guren_Q_Configuration.md)：目标、动画通知、镜头、取消恢复。
- [当前溶解效果](../Docs/VFX/RadiantDissolve_FinalScatter.md)：共享特效配置。

同一红莲资产只保留此 R19 `.blend` 和上述红莲说明、参考索引共四份 Markdown；旧版、阶段 Brief、导出副本、脚本、预览和回退文件不再保留。后续修改以用户当前任务为准。
