# ThiefHound / 偷矿兽 Asset Brief

本文件从源资产目录的既有 Brief 整理入库。当前阶段以第 11 节 P06A 为准，第 8 节保留 P02 历史约束；本次归档不新增建模、动画或 UE 导出授权。

## 1. 基本信息

- Asset Name：ThiefHound / 偷矿兽
- UE Name：SK_ThiefHound
- Asset Category：敌对四足机械单位（未来 Skeletal Mesh）
- Priority：High
- Intended Folder：`/Game/MineLearning/Characters/Enemies/ThiefHound`
- Brief Status：Current
- Authoritative Blender File：`ArtSource/ThiefHound/ThiefHound_P06A_AnimationRecovery.blend`
- Current Stage：P06A Animation Recovery / 动画恢复验收

## 2. 游戏用途

- 玩家识别：低伏、前倾、暗红主色、红色敌对眼灯、外挂开放货架。
- 行为用途：潜入矿区寻找 Pickup，偷取后把动态货物挂到背部并快速逃跑；死亡后由 UE 恢复赃物 Pickup。
- 玩法角色：小型敏捷骚扰怪，不是重型战犬或战斗主怪。
- 阵营区分：暗红身份壳体与红色发光，明确区别友军的工业黄、枪灰与青色灯。

## 3. 风格目标

- 功能角色：敌对拾荒机械猎狗。
- 已批准新身份色：磨损暗红 / 工业红。理由：敌对阵营与高威胁识别。
- 关键词：小、低、快；敌对；偷窃。
- 项目一致性：继续共享 MineLearning 的 Q 版圆润硬表面、枪灰机械骨架、银色工具件、深橡胶关节和清楚的三级结构。
- 避免：人形、友军工程机、处理机、重型 Boss、写实废土、嘴前巨大钳子、封闭背包或高耸笼架。

## 4. 尺寸与比例

- 总长目标：130–160 cm。
- 肩高目标：65–80 cm。
- 总宽目标：50–65 cm。
- 正常站姿：低于 1.8m 玩家，明显低伏前倾。
- 主要观察距离：中景和近景。

## 5. 结构与功能

- 大形体：低伏躯干、长机械犬科头、四条外露机械腿、独立外挂货架。
- 头部：明确吻部、独立机械下颚、红色双眼、两块向后装甲鳍。
- 货架：独立枪灰框架、浅托盘、横梁、侧限位架、橙色卡扣，与躯干存在安装间隙。
- 货架有效区域目标：约 80 × 60 cm。
- 正式模型不得包含固定矿石、箱子或金币。
- Pivot / Forward：世界原点为地面根节点参考；`+X` 为前进方向，`+Z` 为上。

## 6. 色彩与材质

- 主色：暗工业红 / 磨损红色喷涂外壳。
- 公共材料：Gunmetal、Tool Silver、Dark Rubber。
- 辅助色：少量 Rust Orange、暗铜、脏黄警示。
- 发光：红色 / 深红色，只用于眼睛和敌对状态灯。
- 禁止大面积 Cyan Emissive。

## 7. 技术交付

- 最终类型：Skeletal Mesh；本轮仍为模型确认阶段。
- 预计绑定：简单骨架、机械件刚性蒙皮。
- 本轮创建 UE 辅助点：`CargoDisplayPoint`、`HeadPoint`、`BodyCenterPoint`、`VFX_HitPoint`、`VFX_DeathPoint`。
- `CargoDisplayPoint` 位于货架有效区域中心。
- Blender 是静态外观 Source of Truth；UE 负责 Pickup 表现生成、货物挂载/掉落、特效、移动与 Gameplay。
- 测试矿石只能存在于 `90_Review` Collection，不得合并进正式模型。

## 8. 当前 P02 任务范围

- 必须完成：犬科头部定型、独立 Jaw 结构、躯干三级结构、四腿明确旋转中心、脚掌接地、外挂货架定型、中/大型货物验证、八张审阅图。
- 禁止提前制作：Armature、权重、Action、Idle、Run、Hit、Death、Steal、FBX、UE 导入与 Gameplay 逻辑。
- 必须保护：独立头/下颚/四肢机械件结构、货架有效空间、辅助点命名、独立 Review 测试矿石。
- 候选数量：1。
- 停止点：完成 P02 Rig Ready 造型验收后停止，等待用户确认。
- 用户批准后才能继续：Rig、刚性蒙皮、基础动画。

## 9. 本轮验收重点

1. 多个视角都能读成小型低伏敌对机械猎狗，而不是友军机器人或重型战犬。
2. 背部货架明显是后装独立设备，足以显示一件中大型动态货物。
3. 正式模型为空货架，`CargoDisplayPoint` 位置清楚，测试矿石严格隔离。

已知风险：四足比例既要保持 Q 版结实，也不能因大脚和大护甲变成重型单位；货架必须足够大，但不能破坏快速低伏轮廓。

## 10. P06 Appearance Refine（Current）

- 当前 Source of Truth：用户本轮 P06 要求、`References/ThiefHound_Concept_P06.png`、现有 Rig/动作兼容性。
- 阶段文件：`ArtSource/ThiefHound/ThiefHound_P06_AppearanceRefine.blend`；不得覆盖上一阶段 `ThiefHound.blend`。
- 目标：保留小型四足偷窃单位、开放货架、暗红身份色与现有动画逻辑，明显升级犬科头部、前低后高轮廓、躯干分层、机械腿/脚掌及货架连接。
- 参考图只提供外观语言；不制作扫描器、武器系统、固定矿物、嘴部钳子或电影级超高复杂度。
- 必须保护：16 根现有 Bone 的名称/父级/Rest Pose、刚性 Vertex Group、Armature Modifier、Cargo/Guide 点，以及 `Idle / Walk / Run / Hit / Death` Action。
- 允许：在新 P06 文件中调整或替换外壳几何、增加按单骨骼刚性绑定的新外观件、细化材质分区；若动作穿插，仅允许局部外壳修形，不改动作逻辑。
- 停止点：完成多视图、头/腿/货架特写和动作兼容检查后暂停；不导出 UE，不进入下一阶段。

## 11. P06A Animation Recovery（Current）

- 当前源文件：`ArtSource/ThiefHound/ThiefHound_P06_AppearanceRefine.blend`。
- 阶段文件：`ArtSource/ThiefHound/ThiefHound_P06A_AnimationRecovery.blend`；不得覆盖 P06。
- 本轮只整理现有 Rig / Action / Slot / NLA / 编辑器归属状态，不继续修改外观、骨架、权重或关键帧。
- 必须保护：当前 P06 外观、16 根 Bone 的名称/父级/Rest Pose、全部刚性绑定，以及 `ThiefHound_Idle / Walk / Run / Hit / Death` 的关键帧数据。
- 正式驱动对象必须唯一为 `RIG_ThiefHound`；五套动作作为独立 Action 保留并启用 Fake User。
- 若 NLA 并非播放或保存所必需，保持正式 Rig 的 NLA 为空，避免额外混合覆盖；通过显式 Action Slot 绑定与正确的活动 Armature 解决 “unassigned”。
- 停止点：五套动作逐一播放验证、保存 Action Editor / NLA 状态截图后暂停；不重做动画、不导出 UE。
