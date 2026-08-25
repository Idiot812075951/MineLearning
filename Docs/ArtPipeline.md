# MineLearning Art Pipeline

美术工作的入口和 Source of Truth 路径为：

`AGENTS.md`
→ `mining-game-3d-asset-production`
→ `ArtBriefs/<Asset>.md`
→ Art Direction 与按需 Reference
→ Blender / UE
→ 适用验收

## 各层职责

- `AGENTS.md`：选择通用美术 Skill，并声明项目级安全入口。
- 通用 Art Skill：长期复用的安全、建模、材质、版本、Rig/Animation 和 UE 交付原则。
- `ArtBriefs/`：单个资产当前有效的功能、设计、阶段、保护项和交付需求。
- `references/art-direction.md`：全项目视觉、身份色和公共材质 Source of Truth。
- 其他 Reference：只在 Rig、Animation、验收或 UE Export 等对应任务中读取。
- Blender：可编辑几何、静态外观、Rig 和 Animation 的源文件；UE 负责已批准的运行时动态表现与集成。

## Asset Brief 生命周期

- 新资产或重大改版从
  `.agents/skills/mining-game-3d-asset-production/assets/asset-brief-template.md`
  建立一份当前 Brief。
- 同一资产只保留一份明确标为 Current 的 Brief。被替代且仍有追溯价值的版本移入 `ArtBriefs/Archive/`；无价值的临时 Prompt 或 TODO 直接删除。
- 具体尺寸、颜色、部件、版本名、动画帧数和单轮施工要求只写入 Brief，不进入通用 Skill。

## 工作流

1. 读取 `AGENTS.md`、通用 Skill、当前 Brief 和 Art Direction。
2. 按任务类型读取最少的额外 Reference。
3. 确认正确源文件并建立回退点，只执行 Brief 的本轮范围。
4. 在 Blender 完成源设计；需要 UE 交付时再执行导出检查和引用验证。
5. 使用适用验收门槛，报告源文件位置、修改、验证、风险和下一步。

游戏玩法、C++、Blueprint 运行时逻辑和关卡配置应记录在各自技术文档中，不写入美术 Skill 或 Art Brief。
