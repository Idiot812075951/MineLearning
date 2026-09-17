# 红莲外观诊断与修复分工

日期：2026-09-14。本轮先完成原因分析与修复方案，未修改生产模型、材质、骨架、动画或 UE 资产。

## 检查范围与来源

- 用户指出：Blender 白色亮条、胸口绿窗太小且黑圈太大、翅膀廉价。
- 实际连接的 Blender：5.1.2，`ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R16_QGrabHeadIK.blend`，`Guren_Current`，F81，文件有未保存修改。诊断读取的是实时状态。
- UE：读取正式 `Skeletal/SK_Guren`、`M_Guren_Wing`、`MI_Guren_EnergyWing` 及导入设置。
- 比较用户六张截图、项目官方设定图 `References/official_r2_mecha_01c.jpg`、动画能量翼图 `References/ref_09_anime_wing_front.png` 和胸甲产品近照。产品近照用于结构观察，不将其金属涂装或额外刻线直接当作目标。
- 延续已批准的红／橙金／绿／青／银／粉色身份和现有整体比例。通用 Q 版规范不覆盖该资产已有动画风格。
- 进行了同一视角 Cavity 开／关对照及材质预览。最后恢复原有 SOLID + Cavity BOTH 显示，未保存 `.blend`，保留用户的未保存状态。

## 1. 白条：显示增强与真实表面问题叠加

### 已验证

实体视图为 `SOLID`、`STUDIO`、`paint.sl`、`MATERIAL` 颜色；`show_cavity=True`、`cavity_type=BOTH`，四项 Ridge/Valley 均为 1。关闭 Cavity 后，肩甲、腰甲、腿甲及翼骨的很多细亮条消失。它们属于视口强化凸边的效果，不是裂缝提示，也不直接表示缺面。

仍存在的块面转折、宽反光、胸甲格状起伏不能归给 Cavity。材质预览中胸甲与绿窗周围依然可见不连续的表面观感。

面数并不短缺：

| 样本 | 实时源结构 | 修改器后／UE 状态 |
|---|---|---|
| 当前全身 | 204 个网格，39,580 基础顶点 | UE source model 为 347,810 三角形 |
| 左胸红框 `Guren_Chest_Frame_L` | 192 个四边面，384 边，136 条标记硬边 | 31,784 个面；全为 smooth，但保留硬边需单独检查 |
| 左肩 `Guren_Shoulder_L` | 22 面，40 条标记硬边 | 7 段倒角后 1,068 面，面平滑标志均关闭 |
| 腰部正面甲 `Guren_Pelvis_Armor_Front` | 16 面 | 3 段倒角后 210 面，面平滑标志均关闭 |

左胸红框使用 `Bevel 0.007 m / 3 segments / angle 0.12 rad（约 6.9°）`，然后 Catmull-Clark 2 级。384 条边中有 264 条双面边的夹角超过该阈值。对原本应连续的曲面转折进行密集倒角、再叠加细分和硬边标记，是需要逐项隔离验证的重点。高细分不自动消除错误的表面结构。

抽查胸框、橙金胸甲、肩甲及腰甲，均没有非流形边。这排除了这些样本中明显的开裂／缺面，但不是全模型几何无缺陷的保证。

UE LOD0 构建设置开启 `bRecomputeNormals`、`bRecomputeTangents`、`bComputeWeightedNormals`；导入方法为 `FBXNIM_ComputeNormals`。导出脚本本来记录了 Blender 的 corner normals，UE 当前选择重新计算，因此两端反光未形成一致的法线交付规则。该设置可能改变反光，尚未通过另一份 UE 测试网格的 A/B 验证其影响程度。

### 修复方向

- 在关闭 Cavity 的实体视图和材质预览中判断表面质量；Cavity 仅作为结构检查辅助。
- 分别处理平板、折板、曲面：大平面保持平整，设计折线保持明确，倒角带与连续胸壳曲面获得连续法线。
- 单独检查胸框的硬边传播、倒角选边、曲面控制线和局部夹皱。先简化错误的曲面结构，再决定局部细分量。
- 先用一个肩甲样本验证倒角和平滑策略，再按零件类别扩展；不执行全身一键平滑、全身细分或全量 Weighted Normal。
- 用独立 UE 测试资产比较导入源法线与当前重算法线，确定稳定方案后才更新正式网格。

“锯齿”需要区别：轮廓上的像素台阶涉及抗锯齿；曲线轮廓的多边形折线涉及几何采样；表面上的亮条／碎反光涉及倒角、法线、材质和照明，三者不能混为一谈。

## 2. 胸口绿窗：有效绿色面积小，暗边叠加

### 已验证

- 绿窗使用 `M_V026A_LensOptical`。Blender 的 `UV_LensOptical → V026A_Lens_OpticalMask → ColorRamp` 同时驱动 Base Color 与 Emission Color。
- UE 现用 `UEExport/P01/Textures/T_Guren_Lens_BaseColor.png` 为上述效果转换出的 512×512 纹理。视觉上中央只有一块绿色，周围大面积近黑。黑圈不全是实体模型边框，镜片表面本身也有暗边。
- 另有实体 `Guren_Chest_Sensor_Bezel_L/R`，与暗纹理形成多层收缩。
- 实体视图只显示材质显示色，不反映这张绿窗纹理，因此 Blender 实体视图与 UE 看见的绿色面积不同。

### 修复方向

让绿色主体沿现有杏仁形开口展开，深色区域收成窄边和内凹阴影；保持一层清楚的装甲边框，避免黑纹理边与厚框重复包围。先在现有开口内调整有效绿区和 UV／Mask，再判断是否需要小幅扩大镜片或减薄框体。

绿色镜片应有覆盖主体的深浅绿、克制的反射和小面积高光。当前中央椭圆亮斑容易被读成宝石或塑料眼珠。静态纹理与尺寸在 Blender 源资产中成立，再同步 UE；UE 单独控制状态亮度。不要只提高发光强度来掩盖面积问题。

## 3. 翅膀：能量膜被做成均匀发亮的薄板

### 已验证

- 8 片能量翼为有前后面的薄实体。上翼基础 12 顶点／8 面，叠两层单段倒角后为 88 面；源没有专用翼膜 UV。
- 上翼当前包围尺寸约 2.486 × 0.026 × 1.070 m，观感来源是薄板加红色机械翼骨。
- UE 实际分配的父材质为 `M_Guren_Wing`：Translucent、Default Lit、Two Sided、Surface Per Pixel Lighting；不是透明模式漏设置，也不是“粉色就说明贴图丢失”。
- 材质图只有 2 个 Vector Parameter 和 3 个 Scalar Parameter。整片使用 BaseColor、Metallic、Roughness、Emission、Opacity，缺少翼根到翼尖／翼边到内部的变化。
- 实际 Opacity = 0.54、Roughness = 0.23，Emission RGB 约 (0.95, 0.0114, 0.285)。发光数值没有达到可直接称为极高的程度；核心问题是大面积同色同亮，和实体翼片的分面共同造成板材感。
- 两面实体配合双面半透明存在前后表面重叠混合的风险；翼片相交和运动中的排序仍需专门测试。

### 修复方向

Blender 保留机械翼骨、翼根关节、展开轮廓和骨骼驱动，优化能量膜载体及其专用 UV／Mask。能量区域应读作从翼骨延展出的膜，避免厚塑料边和密集倒角。

UE 建立有层次的翼膜：窄而明确的亮边，较透明的内部，翼根至翼尖的渐变，少量沿翼方向流动的能量纹路，以及受控的 Bloom。先完成静态层次，再接飞行状态的流动／脉冲；不能通过把整片发光调到很高来获得完成度。采用透明度与发光分布共同塑形，机械翼骨仍保持实体材质。最终在白天测试场、暗背景、侧视和移动镜头中检查排序与闪烁。

## 4. 材质统一与工作顺序

新旧涂装并存，例如胸框 `M_V026A_PaintedRed` 为 Metallic 0.10／Roughness 0.38，肩甲 `M_Guren_Red_Highlight` 为 0.28／0.28，后者还带 Coat 0.20／Coat Roughness 0.03。肩甲更容易产生又窄又亮的漆面高光。身份色可以保留，涂装应统一为可读的大面积漆面，裸露银色工具、暗色关节、镜片和能量膜各有不同响应。

| 优先级 | 工作 | 主要位置 | 验收方式 |
|---|---|---|---|
| 1 | 绿窗面积、暗圈与胸甲连续表面 | Blender | 同相机的正面、3/4、侧面；中性照明与移动高光 |
| 2 | 翼膜载体、UV 与静态能量层次 | Blender + UE | 亮边／透明内部清晰；背面与交叠无明显板材感 |
| 3 | 肩、腰、腿甲的倒角和硬边分类整理，统一红色涂装 | Blender | 小样通过后逐类扩展，轮廓和接缝仍清楚 |
| 4 | 源法线传递、材质映射、曝光和飞行状态效果 | UE | 与源资产对照，正常游戏距离和不同光照下复查 |

后续制作以当前实时 R16 为起点先建立独立版本和保护基线。保留现有整体比例、骨架、Rest Pose、Action、IK、父子关系、枢轴、权重和材质槽顺序；任何翼膜 UV 改动限定到目标载体。不要把旧导出副本或历史 Prompt 当作最新源。当前没有交付改版 `.blend`、重导入网格或 UE-ready 结论。

## 原理参考

- [Blender Viewport Shading：Cavity 强化 ridges 与 valleys](https://docs.blender.org/manual/en/latest/editors/3dview/display/shading.html)
- [Epic：Material Inputs，Opacity 与 Emissive 的用途](https://dev.epicgames.com/documentation/unreal-engine/material-inputs-in-unreal-engine)
- [Epic：Using Transparency in Materials](https://dev.epicgames.com/documentation/unreal-engine/using-transparency-in-unreal-engine-materials)
