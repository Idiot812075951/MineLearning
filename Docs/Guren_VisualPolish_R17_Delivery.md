# 红莲 R17 美术修复交付

2026-09-14。已更新 Blender 源文件、UE 正式角色网格、材质和 `L_Guren_Retarget_Test` 测试关卡。最终判断采用该关卡的 PIE 实际渲染。

## 文件与回退

- 当前源文件：[V027A_R17_VisualPolish.blend](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R17_VisualPolish.blend>)。
- 导出与制作记录：[VisualPolish_R17](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/VisualPolish_R17>)。其中 `author.py` 为源模型修改配方，`export_r17.py` 为独立刚性蒙皮导出，`ue_author.py` 为 UE 材质及导入配方。
- 实际 FBX：`VisualPolish_R17/SK_Guren_R17.fbx`。
- 正式 UE 网格：`/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren`。仍引用原 `SK_Guren_Skeleton` 和 `ABP_GurenLocomotion`。
- 原实时 Blender 状态（含开始时未保存修改）：`VisualPolish_R17/00_UserLive_R16.blend`。
- 原 UE 网格、材质、纹理和地图备份：`VisualPolish_R17/UE_Backup/`。需要整体回退时，在关闭 UE 后恢复相应 Content 路径；Blender 可直接打开原状态备份。原 R16 文件未覆盖。

## 实际修改

1. **胸甲反光连续性**：两侧胸甲框移除不合理的分裂法线/锐边标记，停用曲面细分前过密的低角度倒角。保留原细分曲面与部件绑定，消除格状、碎片式高光。
2. **装甲倒角和法线**：先验证独立肩甲样本，再处理 52 个对应装甲和结构件。整理重复倒角，控制边宽和段数，采用加权面法线；保留设计中的主要机械折线。对象清单见 `surface_changes.json`。
3. **胸口透镜**：重映射原光学遮罩，提高周边绿色覆盖率，减弱黑边主导的视觉印象；降低刺眼镜面亮斑。原 UV 保留，UE 使用由同一 ColorRamp 生成的 `T_Guren_R17_Lens_BaseColor.png`。
4. **材质统一**：红色与深红色装甲统一为较克制的漆面反光，降低旧材质的金属感与过亮涂层；同步调整橙金和银色。参数由 Blender `material_manifest.json` 传递至 UE。
5. **能量翼**：八片厚翼片改为单层载体，保留名称、外轮廓、原点、变换和骨骼父级；新增专用 UV 和边缘/展向顶点色。UE 采用双面透明自发光，形成亮边、透明内部和轻微流动纹理；红色机械翼架保持不透明。
6. **UE 导入**：保留源法线与切线，关闭重新计算。先导入独立候选并比对，再替换正式网格。材质槽仍为原顺序的 19 槽。
7. **关卡配合**：太阳光强 4、俯角 -38°、朝向 -145°、光源角 3°；天空填光 1.25，Bloom 0.25。补齐测试场后方背景墙。灯光配方见 `ue_lighting_recipe.json`。展示截图使用临时相机和角色位置；运行时测试结束后已退出 PIE。

## 验证与证据

- 源模型仍为 204 个网格部件；原两套骨架数据及 63 个 Action 哈希比对一致。原零件父级、骨骼父级、变换、材质槽和顶点组均未改变。见 `source_validation.json`。
- 导出为 59 骨骼、117,867 几何顶点、234,932 三角面；三角面相较原约 347,810 降低约 32%。单位为厘米，模型高度约 4.65 米。
- 候选 UE 网格与原网格骨骼名称/顺序一致；参考姿态最大位置/缩放/四元数分量误差约 0.0000824（位置单位厘米），远低于 0.01 验收阈值。见 `ue_candidate_validation.json`。
- 正式角色在 PIE 中使用更新后的 `SK_Guren` 和原动画蓝图；完成正面、侧面、背面、胸部近照及飞行、普攻、Q 抓取、收翼姿态采样。可见部件随原骨骼运动，未见新增脱离、错位或重复翼壳。见 `ue_pose_validation.json`、`ue_hero_cameras.json`、`ue_delivery.json`。
- 最终三个材质父级重新编译并保存；正式网格保留源法线/切线设置。地图和正式资产均已保存。

实际截图：[正面整身](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/VisualPolish_R17/11_UE_Hero.png>) · [胸部](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/VisualPolish_R17/13_UE_Chest.png>) · [背面](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/VisualPolish_R17/12_UE_Back.png>) · [收翼](<C:/Users/gh/Documents/Unreal Projects/MineLearning/ArtSource/Characters/GurenSeitenHakkyoShiki/VisualPolish_R17/15_UE_folded.png>)。

## 评审范围

按本轮 Brief 保留红莲既有动漫比例和尖锐机械轮廓。适用最终评分：轮廓 2、风格 2、功能 2、细节 1、材质 2、技术 2、可维护性 2、扩展性 1，共 14/16；本轮修复范围内未发现硬失败。

近距离仍可见既有装甲主折线和头部/手部原有细节密度。透明翼膜收拢后会叠色，极侧视呈细亮边；这些表现已在背面、侧面和收翼截图中核对。此轮验证为美术及动画姿态验证，没有重新执行全部战斗逻辑自动化。完整的自定义碰撞/物理资产制作也不属于本轮范围，原网格 PhysicsAsset 为空的状态保持不变。
