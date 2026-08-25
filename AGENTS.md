# MineLearning 项目指导

## 美术资产工作

- 处理 Blender 建模、材质、角色/机器人、Rig、Animation、静态或可动设备、以及 UE 美术交付任务时，必须使用 `$mining-game-3d-asset-production`。
- 默认读取通用 Skill、当前 Asset Brief 和 Art Direction；Rig、Animation、验收与 UE Export Reference 仅在对应任务中按需读取。
- 新资产或重大改版必须有一份当前有效的 Asset Brief；已有资产的小型修改至少明确本轮范围、Source of Truth 和保护项。
- Blender MCP 无法连接、连接到错误文件、无法确认绝对路径或会覆盖唯一 `.blend` 时，立即停止写入并报告。
- Blender 被指定为静态外观 Source of Truth 时，不得依赖 UE 内不可追溯的手工改色或改形覆盖源设计。
- 优先小步修改、阶段保存、可回退和可验证的方案；不得无理由清空场景、推倒已确认资产或破坏 Rig、Animation、UV 与导出结构。
- 只有完成适用验收与导出检查后，才能宣称资产完成或 UE-ready。
