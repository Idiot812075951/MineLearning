# MineLearning 美术文件入口与保留清单

## 当前规则

美术工作从 `AGENTS.md`、`mining-game-3d-asset-production`、当前 Brief 和 Art Direction 进入。Blender 是可编辑外观和动画的源文件，UE 负责运行时装配与表现；既有骨架、动作、绑定和材质保护规则继续适用。

同一功能跨目录合计只保留最新有效的 **1 个 `.blend`、最多 5 份必要 Markdown**。旧版本、阶段 Brief、备份、检查点、导出副本、一次性脚本、JSON/DSL 记录与预览验收图片/视频全部清理。版本历史由 Git 管理，不另建 Archive 或回退目录。

概念图、设计参考图、当前实际使用的贴图和 UI 源图可以保留；按用途筛选，不按 PNG 扩展名删除。新增参考图应单独审查后选择入库，不能把整个输出目录加入 Git。UE `Content` 正式资产、业务代码和技能自身支持文件不受此源资产文件限额约束。

## 唯一 Blender 源文件

| 功能 | 最新有效文件 |
| --- | --- |
| 搬运机器人 | [CarrierRobot_V08_ArmPosePolish.blend](../ArtSource/Characters/CarrierRobot/CarrierRobot_V08_ArmPosePolish.blend) |
| 红莲 | [V027A_R19_Refinement.blend](../ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R19_Refinement.blend) |
| 矿区环境 | [MiningArea_P2_Environment_Source.blend](../ArtSource/Environment/MiningArea/MiningArea_P2_Environment_Source.blend) |
| 机器人中心 | [RobotCenter.blend](../ArtSource/Environment/RobotCenter/RobotCenter.blend) |
| 铁锭 | [IronIngot.blend](../ArtSource/IronIngot/IronIngot.blend) |
| 金币 | [SM_GoldCoin.blend](../ArtSource/Items/GoldCoin/SM_GoldCoin.blend) |
| 矿石处理机 | [OreProcessor_V26_InclineBeltCleanSlab.blend](../ArtSource/OreProcessor/OreProcessor_V26_InclineBeltCleanSlab.blend) |
| 出售处 | [SellStation.blend](../ArtSource/SellStation/SellStation.blend) |
| 偷矿兽 | [ThiefHound_P06A_AnimationRecovery.blend](../ArtSource/ThiefHound/ThiefHound_P06A_AnimationRecovery.blend) |
| 仓库 | [Warehouse_V06_UEExportReady.blend](../ArtSource/Warehouse/Warehouse_V06_UEExportReady.blend) |

## 必要文档

- 红莲共五份：[当前 Brief](../ArtBriefs/GurenSeitenHakkyoShiki.md)、[R19 交付](Guren_Refinement_R19_Delivery.md)、[Q 配置](Guren_Q_Configuration.md)、[大招降临](Guren_Ultimate_Arrival.md)、[设计参考索引](../ArtSource/Characters/GurenSeitenHakkyoShiki/References/REFERENCE_INDEX.md)。
- 偷矿兽：[P06A 当前 Brief](../ArtBriefs/ThiefHound.md)。
- 矿区环境：[P2 导入要求](../ArtSource/Environment/MiningArea/References/MiningArea_UEImportBrief.md)。
- 机器人中心：[当前布局要求](../ArtSource/Environment/RobotCenter/References/RobotCenter_P03_LayoutBrief.md)。
- 铁锭：[当前 Brief](../ArtSource/IronIngot/IronIngot_AssetBrief.md)。
- 出售处：[当前 Brief](../ArtSource/SellStation/References/SellStation_AssetBrief.md)、[结构接入语义](../ArtSource/SellStation/References/SellStation_UEIntegrationNotes.md)。
- 矿石处理机：[当前 Brief](../ArtBriefs/OreProcessor.md)；业务说明继续使用现有 `Docs/Logistics`。
- 辐射溶解：[当前 Brief](../ArtBriefs/RadiantDissolve_FinalScatter.md)、[参数与使用说明](VFX/RadiantDissolve_FinalScatter.md)。该功能以 UE 资产为源，不需要额外 `.blend`。
- 红莲降临：`Content/MineLearning/Characters/Guren/Ultimate` 内的三套 Niagara、四个专用材质及 `SM_ArrivalFragment` 为正式可编辑源；红色喷散现用于贯穿命中。最终白色溶解直接复用 `VFX/RadiantDissolve` 的 Q 表面函数和两套 Niagara，不复制 Q 资源；后续轮廓修复仅将共享静态系统的采样来源改为优先使用实际目标组件，保留视觉参数。配置与验收写入现有大招说明，不增加 Blender 文件或阶段文档，不保留一次性制作器、脚本、导出副本与验收帧。

## 保留图片的用途

- `ArtSource/Concept/OreBuddy/OreBuddy07_Prototype.png`：OreBuddy 概念参考，保留。
- 红莲 `References` 中的官方/动画/商品/用户原始图片及手部结构图：用于比例、结构与动作设计；重复原图和联系表已移除。
- 机器人中心、出售处和偷矿兽 `References`：保留概念图及有功能标注的设计动线图。
- `ArtSource/UI/Gunner`：只保留金色 v001、银色 v002 正式源图；Chroma 中间图和旧银色 v001 已移除。
- `ArtSource/UI/Transformation`：保留 Human、Gunner、OreBuddy 三张正式头像源图。
- `ArtSource/UI/Guren`：保留正式 Q 爪子图 `T_GurenGrabClaw.png`，以及实际 R 降临阶段渲染的幻化头像 `T_TransformPortrait_Guren.png`（768×1024）、技能近景 `T_GurenArrival.png`（512×512）。两张新图共用实际降临姿态与修订后的日蚀光环，各自构图；不保留旧图、制作脚本或验收截图。

清理前已只读打开十个保留的 `.blend`，没有对待删除文件的外部依赖；红莲镜片图像已内嵌。清理不修改这些 `.blend` 的内容，也不修改当前打开的 Blender 未保存状态。UE 导入产物保持原样，需要重新导入时从上述源文件重新导出，不再依赖旧 FBX。
