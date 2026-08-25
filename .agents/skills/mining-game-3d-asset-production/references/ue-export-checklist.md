# MineLearning Blender → Unreal 交付检查

仅在任务涉及 FBX、UE 导入/重导入、Static/Skeletal Mesh 交付或准备宣称 UE-ready 时读取。

## Source of Truth

- 记录权威 Blender 文件、导出文件和目标 UE 路径。
- 静态造型、基础材质分区和基础颜色在 Blender 源资产中成立；UE 只添加已批准的运行时参数、动画、Panner、灯光或特效。
- 不把 UE 内临时缩放、修色、重命名或手工槽位调整当作源文件修复。

## 通用检查

- 单位、尺寸、Forward/Up 轴和场景原点符合项目约定。
- Object 名称、层级、Origin/Pivot、Transform 和可见导出范围明确。
- 无明显破面、反法线、重复几何、悬空碎片或任务外隐藏对象被导出。
- 材质槽数量、名称、顺序和面分配稳定；UV 层存在且未被意外重建。
- 法线、Shade Smooth、Bevel 和需要保留的 Modifier 在导出策略下结果一致。
- 活动部件保持独立，Pivot 与局部轴适合 UE 驱动。

## Static Mesh

- 确认是否需要拆分导出、简单碰撞、Socket 或模块化拼接 Pivot。
- 模块拼接端、比例和命名可用于关卡搭建。
- 不因导出便利永久破坏 Blender 中可编辑的源结构。

## Skeletal Mesh 与 Animation

- Root、Bone 名称/层级、Rest Pose、Armature Modifier 和 Vertex Group 已验证。
- 只导出需要的 Deform Bone；额外控制 Bone 的处理方式明确。
- Skeleton 与 Mesh 的 Transform、Scale 和轴向一致，不依赖 UE 内补偿。
- 每个 Action 的名称、帧范围、FPS、循环与 Root Motion 决策明确；不会把多个动作意外烘成一段。
- 重导入前确认是否必须保持现有 Skeleton、Material Slot、Socket、Physics Asset 或 Animation 引用。

## UE 验证

- 导入后检查尺寸、朝向、Pivot/Root、材质槽、法线、UV、动画列表和活动件。
- 对重导入资产，检查现有 Blueprint、Animation Blueprint、Material、Socket、Physics 和关卡引用是否仍有效。
- 只有完成适用检查、保存源文件并报告已知风险后，才能宣称“UE-ready”或“可直接导入”。

未明确授权时，只做检查与交付建议，不修改 UE Asset、Blueprint、运行时配置或游戏代码。
