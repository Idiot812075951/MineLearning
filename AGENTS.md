# MineLearning 项目指导

## 项目内默认授权

- 用户已授权：当前任务中，影响范围仅限本项目路径的修改、删除和工具调用，默认批准，无需重复询问。
- 此授权不扩大任务范围；涉及项目路径之外的影响时，另行说明并取得所需授权。

## UI 工作

- 处理运行时 UI 的设计、实现、修改、调试或评审时，必须使用 `$minelearning-umg-ui`。

## 美术资产工作

- 处理 Blender 建模、材质、角色/机器人、Rig、Animation、静态或可动设备、以及 UE 美术交付任务时，必须使用 `$mining-game-3d-asset-production`。
- 处理人形/机甲的姿态、动作节奏、行走/待机/战斗动画质量时，同时使用 `$blender-animation`；处理骨架控制、IK/FK、约束、Driver、Action Slot、FCurve 或 NLA 技术问题时，同时使用 `$blender-animation-rigging`。
- 这两份动画 Skill 是补充指导，不替代当前 Asset Brief、参考图和既有 Rig/Action 保护规则；安装或使用 Skill 本身不授权更换骨架、修改 Rest Pose、创建任务外动画或进行 UE 导出。
- 默认读取通用 Skill、当前 Asset Brief 和 Art Direction；Rig、Animation、验收与 UE Export Reference 仅在对应任务中按需读取。
- 新资产或重大改版必须有一份当前有效的 Asset Brief；已有资产的小型修改至少明确本轮范围、Source of Truth 和保护项。
- Blender MCP 无法连接、连接到错误文件、无法确认绝对路径或会覆盖唯一 `.blend` 时，立即停止写入并报告。
- Blender 被指定为静态外观 Source of Truth 时，不得依赖 UE 内不可追溯的手工改色或改形覆盖源设计。
- 优先小步修改、阶段保存、可回退和可验证的方案；不得无理由清空场景、推倒已确认资产或破坏 Rig、Animation、UV 与导出结构。
- 只有完成适用验收与导出检查后，才能宣称资产完成或 UE-ready。
