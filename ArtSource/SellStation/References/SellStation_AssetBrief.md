# Sell Station / 出售台 — 当前设计 P04

唯一源文件：`ArtSource/SellStation/SellStation.blend`。概念参考为同目录 `SellStation_Concept.png`，结构与接入语义见 [UE 接入说明](SellStation_UEIntegrationNotes.md)。

出售台是固定交易设施，以大屏和开放货面识别，不是处理机或电子秤。中央区域用于动态放置待售物和生成金币；模型不含固定货物、金币、文字、UI、加工孔、机械臂或传送带。

整体约 280 × 220 × 228 cm，货面高约 82 cm，中央有效区约 190 × 145 cm，屏幕有效显示区约 178 × 70 cm。Blender 正面朝 -Y，地面中心为 Pivot，屏幕显示面独立。

身份主色为深蓝灰，配合 Gunmetal、Tool Silver、Dark Rubber、少量暗工业黄和 Cyan 反馈。保留八个现有材质槽及 `SM_SellStation_Body`、`SM_SellStation_Screen` 拆分。

P04 的主识别点是四角成交节点与分段交易光轨；屏幕保留厚框、压顶、承托和根部连接。保护中央货物空间、搬运接近空间、屏幕独立材质及 `ItemDisplayPoint`、`TransactionPoint`、`CoinSpawnPoint`、`RobotApproachPoint` 名称与坐标。

Blender 负责静态外观；UE 负责屏幕、交易、货物和金币表现。后续导出时按实际需要核对 UV、法线、碰撞与导入轴，不通过 UE 手工补偿掩盖源文件问题。

本功能只保留当前源文件、两份说明和设计参考图，不保留旧版、检查点和预览验收文件。
