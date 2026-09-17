# 红莲 R18：连续曲面与哑光漆面

已更新 Blender 源模型和 UE 正式 `SK_Guren`。本轮修复的是主要装甲表面的切块感：肩甲、腰前甲、膝甲、背甲、腰后连接件、大腿外壳与装饰甲、左前臂护甲，共 16 件。R17 的胸部镜片和能量翼设计沿用。

## 文件与回退

- 当前源文件：[V027A_R18_SurfaceFlow.blend](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R18_SurfaceFlow.blend>)。
- 配方、FBX、检查 JSON 和截图：[SurfaceFlow_R18](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18>)。
- FBX：`SurfaceFlow_R18/SK_Guren_R18.fbx`。
- UE 正式网格：`/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren`；候选副本：`/Game/MineLearning/Characters/Guren/SurfaceFlow_R18/SK_Guren_R18`。
- UE 关卡：`/Game/MineLearning/Maps/L_Guren_Retarget_Test`。
- 开工时 Blender 快照：`SurfaceFlow_R18/00_UserLive_R17.blend`；原 R17 文件保留。
- 开工时 UE 网格、材质实例、地图备份：`SurfaceFlow_R18/UE_Backup/Content/`。完整回退时关闭 UE，再恢复对应 Content 路径；不要用 R17 的旧备份回退 R18。

## 实际修改

旧版的微小倒角与加权法线强化了大平面的分界。新版本为指定装甲建立连续的细分曲面，以局部折痕控制重要轮廓。肩甲合并原先的微小角部碎面，消除几条棱线交汇形成的尖亮点；大腿保留原纵向环线，增加横截面的弧度；前臂先合并共面三角分割，再修整曲面。各件保持原有局部包围范围、物体变换和骨骼连接点。所有修改按部件设置，没有全身重网格化。

红色漆面粗糙度由 0.39 调为 0.52，金色 0.46，深色结构件 0.54，指定材质的高光参数 0.27；关闭相应漆面的清漆层。参数先写入 Blender，再传到 UE 材质实例并读回核对。镜片、银色机械件和发光翼保持各自的材质区别。

关卡新增 `R18 Armor Bounce Fill`：强度 1.05、轻微偏冷、无阴影的方向填充光；天空填充由 1.25 调至 1.6。原太阳强度 4.0 保持，补光用于改善后背的可读性。详细参数见 `ue_lighting_recipe.json`。

## 检查证据

- [相同光照下修改前的高反射整机](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18/04_Before_Gloss_threequarter.png>) / [修改后](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18/05_After_Gloss_threequarter.png>)。
- [背面高反射检查](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18/06_After_Gloss_back.png>)。
- [UE 正面](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18/11_UE_Hero.png>) / [胸腰近景](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18/13_UE_Chest.png>) / [背面](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18/12_UE_Back.png>) / [收翼](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/SurfaceFlow_R18/15_UE_folded.png>)。

Blender 场景 `R18_Gloss_Inspection_NON_EXPORT` 保留了最终形体在 F81 的独立高反射检查快照，便于转动视角复查；这是诊断副本，不用于动画或导出。`Guren_Current` 是可编辑、有原骨架和动作的正式场景；`R18_Review` 用于最终材质预览。01–03 图片是前期曲面方案试验，最终整机以 04–07 图片为准。

技术检查：204 件原模型对象的绑定、变换、材质槽与 UV 未改变；原骨架和全部 Action 曲线校验一致。16 件修改装甲均为封闭流形，未检测到退化面、非有限坐标或负体积。UE 保留 59 根骨骼、同一 Skeleton 与 19 个材质槽，保留导出的法线和切线；候选与原骨架最大数值差异约 0.000292 cm，正式导入与候选一致。Physics 引用保持原值。

实际 PIE 角色完成待机正面/背面/侧面、飞行、攻击、抓取、收翼与恢复 AnimBP 共 8 个姿态/视角样本检查，未见新增的装甲脱离、明显穿插或绑定异常。`ue_pose_validation.json` 记录正式网格路径与关键骨骼位置。这是模型与姿态回归检查，不代表重新验收全部战斗逻辑。

本轮局部精修评估：轮廓 2、风格 2、功能结构 2、细节 1、色材 2、技术 2、可维护 2、系列扩展 1，共 14/16。已有机械分件与尖角轮廓有意保留；材质仍是干净的风格化漆面。

## 成本与范围

导出三角面由 R17 的 234,932 增至 288,618，约增加 23%；本轮以近景质量为先，没有新增 LOD 或完成目标平台性能测量。骨架、动画、玩法代码与正式相机控制未改。检查时的角色位置、相机和输入限制仅作用于临时 PIE 会话，结束会话后恢复。
