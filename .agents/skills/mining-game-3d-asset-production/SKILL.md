---
name: mining-game-3d-asset-production
description: Create, revise, rig, animate, review, or prepare Blender 3D art assets for the MineLearning Unreal project. Use for characters, robots, ores, machines, tools, props, environments, materials, and UE art delivery. Do not use for coding-only, gameplay-logic, or unrelated UE configuration tasks.
---

# MineLearning 3D 美术生产

## 目标与 Source of Truth

为 MineLearning 制作可持续迭代的 Q 版工业科幻 3D 资产，并保护 Blender 源文件、机械结构、动画数据和 UE 交付稳定性。

- 用户当前明确要求与已批准 Asset Brief 决定单个资产的需求、阶段和保护项。
- `references/art-direction.md` 是跨资产视觉 Source of Truth。
- 当 Blender 被指定为静态外观源资产时，基础颜色、材质分区和静态造型必须在 Blender 中成立；不得用 UE 内不可追溯的手工覆盖代替源设计。UE 可以负责运行时参数、动画和特效。
- 历史 Prompt、旧 TODO、过期版本说明和未批准概念不得覆盖当前 Brief。

## 任务开始时读取

默认只读取：

1. 本 Skill。
2. 当前 Asset Brief；若任务未提供 Brief，使用 `assets/asset-brief-template.md` 建立或补齐最小 Brief。
3. `references/art-direction.md`。

仅在任务确实涉及对应内容时读取：

- 草模评审、最终验收或准备宣称完成：`references/acceptance-rubric.md`
- Armature、Bone、权重、机械关节或绑定：`references/rigging-rules.md`
- Action、关键帧、循环、Root Motion 或动画导出：`references/animation-rules.md`
- FBX、UE 导入/重导入或宣称 UE-ready：`references/ue-export-checklist.md`

不要为了“可能有用”加载无关 Reference。

## 写入前安全门槛

出现以下任一情况，立即停止写入并报告：

- Blender MCP 无法连接，或连接测试失败。
- 无法确认当前 `.blend` 的绝对路径、保存状态或是否连接到正确文件。
- 无法确定哪个版本是当前源资产，或下一步会覆盖唯一可用 `.blend`。
- 不清楚修改是否会破坏 Skeleton、Bone、Action、关键帧、UV、Vertex Group、Armature Modifier、父子关系、Pivot 或材质槽。
- 目标对象、任务范围或必须保留的结构无法从 Brief、场景或用户要求中确认。

写入前必须：

- 只读检查当前文件、场景、Collection、单位、对象层级和任务相关数据。
- 确认一个可回退起点；优先另存版本或阶段检查点，不覆盖唯一源文件。
- 对受保护数据建立与风险相称的基线，例如名称、数量、槽位、UV、Rig 或 Action 摘要。

## 通用设计原则

- 新原型或新 Brief 进入正式制作前，必须回答：资产的功能角色、身份色，以及它与现有 MineLearning 资产并置时是否属于同一项目。
- 先解决大轮廓、比例和功能分区，再做中型功能结构，最后才是少量点缀细节。
- 通过清楚的大块面、明确倒角、厚度、接缝和功能分件获得完成度。
- 新增细节必须服务轮廓、功能、材质层次或状态反馈；不得依靠随机螺丝、管线、凹槽或材质噪声堆复杂度。
- 复杂方案优先采用简单、少而准、可验证、可回退的实现，不为了“高级”引入难以维护的系统。
- 新资产先完成草模门槛；已有资产只修改 Brief 授权的当前阶段，不无理由推倒已确认结构。

## 材质与外观

- 遵循 Art Direction 的公共材质语言和身份色，不为单个资产随意创造新主色体系。
- 使用少量、功能明确的材质族；通过 Base Color、Metallic、Roughness 和受控 Emissive 区分外壳、结构、工具、橡胶与反馈区域。
- 所有机械件不得共享同一种塑料质感；工具件、裸露金属、喷涂外壳和橡胶应可读地区分。
- 磨损与污渍只能轻量、可控，不得把项目推向写实战争机器、废土或脏旧重工业。

## 结构、Rig 与 Animation

- 活动机械部件保持清楚的对象拆分、Pivot、局部轴、父子关系和可解释命名。
- 刚性机械绑定不得依赖未经检查的自动权重；每个刚性部件的驱动关系必须可验证。
- 未经明确授权，不修改现有 Skeleton、Bone、Action、关键帧、Vertex Group、Armature Modifier 或功能部件位置。
- Rig 或 Animation 任务必须读取相应按需 Reference，并在修改前后验证受保护数据。

## UE 交付

- 只有在完成 `references/ue-export-checklist.md` 的适用检查后，才能宣称 UE-ready。
- 保持尺寸、轴向、Root、对象/骨骼命名、材质槽、法线、Transform、UV、动画拆分和活动部件层级可追踪。
- 不把 UE 中临时手工修色、缩放或重命名当作源资产修复。
- 未明确授权时，不修改 UE Asset、Blueprint、运行时配置或游戏代码。

## 工作方式

1. 核对 Brief、源文件、当前阶段、保护项和验收重点。
2. 建立回退点，只执行本轮范围。
3. 小步修改、阶段检查；失败时停止扩散并回到最近可用版本。
4. 验证受影响的数据与常用视角；只修复失败维度。
5. 报告修改内容、文件位置、验证结果、风险和下一步最小任务。

## 停止条件

不得在工具失败、几何损坏、风格明显跑偏、文件版本不明或必须依赖专业人工大修时继续堆叠修改。停止扩散，保留或回到最近可用版本，标出失败对象与步骤，并提出更简单、可验证的替代方案。不得用“看起来差不多”掩盖已知问题。

## 冲突优先级

发生冲突时按以下优先级执行：

1. 用户在当前任务中的明确要求。
2. 当前已批准 Asset Brief。
3. 已批准且可见的项目资产与技术约束。
4. Art Direction 与按需 Reference。
5. 本 Skill 的默认原则。

不得用低优先级规则推翻更高优先级决定。
