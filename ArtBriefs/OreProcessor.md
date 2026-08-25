# Ore Processor Asset Brief

## 1. 基本信息

- Asset Name：Ore Processor
- UE Name：`SM_OreProcessor`
- Asset Category：带简单活动部件的机器
- Priority：当前结构修正
- Intended Folder：`ArtSource/OreProcessor`
- Brief Status：Current
- Authoritative Blender File：`ArtSource/OreProcessor/OreProcessor_V26_InclineBeltCleanSlab.blend`
- Current Stage：Tripo 导入模型的结构整理 / 动画准备

## 2. 游戏用途

- 玩家通过接料箱、斜传送带、顶部料斗和中央处理核心识别矿石处理机。
- 物流路径为：接料箱 -> 斜传送带 -> 顶部短水平接驳段 -> 上方大料斗。
- 后续在 UE 中表现皮带、滚轮和中央核心的运行动画。

## 3. 风格目标

- 功能角色：固定式矿石生产/处理单位，将原矿转换为后续物流资源。
- 身份色：Engineering Yellow / 暗工业黄；理由是它属于持续主动工作的生产机械。
- 关键词：Q 版、明亮、友好且功能清楚。
- 保留当前 Tripo 模型的整体美术风格和主轮廓。
- 避免写实重工业、废土、Low Poly 切面和廉价玩具感。
- 本轮不修改表面轻微裂纹或脏纹。
- 与 OreBuddy、Gunner、搬运工、矿机和仓库并置时，共享大块面、明确倒角、Gunmetal 结构、Tool Silver 功能件、Cyan 状态反馈和少量 Warning Orange。

## 4. 尺寸与比例

- 沿用当前导入模型的尺寸、比例与摆放。
- 观察重点：顶视物流路径，以及前/侧/四分之三视角的结构连续性。

## 5. 结构与功能

- 主体大形体：处理机主机、上方大料斗、侧向斜传送带、底部接料箱。
- 输入位置：接料箱。
- 处理路径：斜传送带与新增顶部短水平接驳段。
- 处理位置：大料斗与中央发光核心所在主机。
- 可活动/可拆部件：斜传送带皮带主体、顶部接驳段、顶部滚轮、底部滚轮、中央发光核心，以及明确合理的小滚轮。
- Pivot：滚轮 Origin 尽量位于自身转轴中心；皮带与接驳段保持独立对象。

## 6. 色彩与材质

- 沿用现有材质与颜色。
- Engineering Yellow 作为身份主色；Gunmetal、Tool Silver 与 Dark Rubber 作为公共结构色，Cyan Emissive 与 Warning Orange 只作功能反馈和少量强调。
- 本轮禁止制作或重做材质、贴图与 UV。

## 7. 变体与复用

- 本轮不制作变体。
- 顶部接驳段采用简单、稳定、可继续修改的独立部件。

## 8. 技术交付

- 目标：后续可拆分进入 UE 的 Static Mesh 部件。
- 本轮不制作骨骼、动画、UV、贴图、重拓扑或大规模减面。
- Blender Source of Truth：`ArtSource/OreProcessor/OreProcessor_V26_InclineBeltCleanSlab.blend`。
- 保留导入基线，并另存本轮清理版本。
- 必须保护现有材质槽、UV、整体比例和已确认主轮廓；UE 只负责皮带、滚轮、中央核心等运行时动态表现。

## 9. 本轮任务范围

- 删除确认无用的 AI 残片、小补片与悬空碎件；若 `tripo_part_35` 为接料箱内无用残片则删除。
- 保留斜传送带主体，删除或废弃顶部错误小块，新补短水平接驳/导料段并接到大料斗边缘。
- 轻量清理接料箱内部，不大改外形。
- 整理后续要转动或单独驱动的对象与 Origin，不制作动画。
- 本轮必须保护：材质/UV、整体尺寸比例、已确认主轮廓和非目标功能件。
- 停止点：结构逻辑、接料箱清理与可动件整理完成并通过本轮验收。

## 10. 验收重点

1. 顶视可明确读出“接料箱 -> 斜带 -> 顶部接驳段 -> 大料斗”，无断开、悬空或送不到料斗。
2. 接料箱内部无明显垃圾残片。
3. 斜带、接驳段、主要滚轮与中央核心基本独立，滚轮 Pivot 位于自身轴心附近。

已知风险：

- Tripo 缺少顶视图输入，顶部结构需依照功能逻辑保守修正。
- 未知碎件不得只凭名称删除，必须结合位置、尺寸和可见用途确认。
