# OreBuddy 采矿与交付

## 职责

OreBuddy 负责采矿与原矿运输，不负责搬运金币。自动模式默认送回仓库；玩家控制时可携矿靠近处理机按 E 直接交付，无需仓库订单。分类限制由 `UResourceCarryComponent` 的 `AllowedCategories = Ore` 实现，不在 AI 中判断角色类。

## 运行流程

1. 空闲时寻找可采矿脉或最近的可用 Pickup。
2. 只有当该 Pickup 当前存在合法接收方时才会预订并前往，避免捡起后无处交付。
3. 到达目标后播放采集动作；动画 Notify 执行真正收集。
4. 携带达到条件后，通过 `UItemLogisticsLibrary` 解析目的地。
5. 使用接收方提供的交付 Transform 发起 MoveTo。
6. 必须先抵达交付位置；抵达后停止移动并进入 `bAligningForDelivery`。
7. 原地旋转至交付 Transform 的朝向，再开始 Deposit 动作；不是从远处倒车入库。
8. Deposit Notify 调用 `IItemReceiver::AcceptItem`，成功后清空携带组件。

## 核心 C++

- `MiningCompanionAIController.*`：状态机、寻路、对齐、动作 Notify 和交付。
- `MiningCompanionTargetingComponent.*`：过滤不可携带或无合法目的地的 Pickup。
- `MiningCompanionCharacter.*`：Ore 分类携带策略和 Pawn/导航碰撞配置。

## 关键变量

| 变量 | 当前值/作用 |
| --- | --- |
| `DeliveryAcceptanceRadius` | `65 cm`，到达交付位置的接受半径 |
| `DeliveryRotationSpeed` | `180°/s`，抵达后原地转向 |
| `DeliveryRotationTolerance` | `1°`，完成转向的容差 |
| `CollectAnimationPlayRate` | `4.0`，玩家 R 与自动 AI 共用，AI 相比之前的 2.0 再加速一倍 |
| `DirectMoveSpeed` | `200 cm/s`，窄通道恢复移动速度 |
| `NavigationStallTimeout` | `1.0 s`，无进展后进入本地恢复移动 |

## 碰撞与导航

OreBuddy 胶囊忽略 Pawn，并且不影响 NavMesh；搬运工也使用同样策略，避免两者在机器狭窄通道内面对面锁死。正常路径失败或持续无进展时，AI 会在局部使用确定性的直线恢复移动，到达后仍执行“停止 → 原地旋转 → 交付”。

## 拾取物尺寸与装载外观

`BP_OreBuddy07` 是运行时装配比例的配置入口，玩家幻化和自动 OreBuddy 共用它。骨骼网格相对缩放为 `1.35`，相对位置 Z 为 `-86.81012 cm`，履带底部对齐既有胶囊底部；胶囊、导航尺寸、骨架、动画和抓取挂点保持原配置。

原矿和铁锭仍使用 `MineLearningItemVisual` 的最长边 `30 cm` 标准，金币为 `42 cm`。抓取中的物体保持世界尺寸；背包装载显示也抵消挂点的缩放，不能随机器人一起放大矿石。

背包四个槽位改为两层、每层两块。`ResourceCarryComponent.PreviewResourceTransforms` 相对 `S_CargoBin`：X 均为 `0.274517`，Y 为 `-0.17 / 0.09`，下层 Z 为 `-0.28`，上层 Z 为 `-0.47`。当前挂点世界缩放为 135，因此列间距约 `35.1 cm`、层间距约 `25.65 cm`，容纳现有两种原矿外形。挂点包含导入骨骼缩放，以上局部值不能直接当世界厘米填写。

已在矿区 PIE 检查实际 R 抓取、抬起和四块满载，并通过 `MineLearning.Demo.NavigationContract` 与 `MineLearning.Demo.PhysicalProduction` 回归。玩家 R 的 4 倍播放率保持不变；本次仅修改蓝图装配和装载槽位，不修改源网格、Rig 或动画。

## 履带运行材质

`M_OB07_Track` 是左右履带实例共用的运行材质；继续使用现有 `TrackOffset` 驱动前后移动。分段轮廓为平整块面、斜边和暗槽，切线法线、粗糙度及 AO 共用同一轮廓，避免只有颜色条纹。`TreadRepeat = 18` 控制节数，`TreadBevelStrength = 2.4` 控制斜边法线强度。保留既有溶解材质函数、源网格、UV 和动画。

拾取速度统一配置在 `BP_OreBuddy07Controller.CollectAnimationPlayRate`，玩家不再额外乘倍率，自动 AI 与玩家均通过动画通知完成抓取和入舱。

## Shift 冲刺与体力

本轮范围是玩家 OreBuddy 的局内冲刺、HUD 和已有履带的运行时滚动驱动；Source of Truth 为 `AMiningCompanionCharacter`、`BP_OreBuddy07.UpdateTrackScroll` 和 `WBP_OreBuddySprint`。源网格、骨架、动画、UV、履带材质的凹凸效果及拾取配置保持不变。

- 按住左/右 Shift 并移动即可冲刺，速度从 200 平滑升至 360 cm/s（1.8 倍），提速 0.35 秒，松开或耗尽后 0.45 秒回落。
- 满体力约支持 3 秒持续冲刺；停止消耗后等 0.8 秒，随后 4 秒回满。静止、腾空、采矿/装卸动作及打开菜单时不消耗冲刺体力。
- 耗尽后不会在持续按住 Shift 时反复触发短冲刺；松开 Shift 且恢复至少 20% 后可再次使用。
- 技能栏上方显示专属体力条，正常/恢复为青蓝色，冲刺中为黄色；换到其他形态隐藏。Widget Blueprint 绑定角色状态事件和控制器换 Pawn 事件，成对解绑，无 UI Tick。
- 左右履带按 `前向实际速度 × DeltaSeconds ± 转向弧长` 除以周长计算 UV 增量，不再除以 MaxWalkSpeed，也不将实际速度比例限制在 1。当前 1.35 倍装配对应视觉校准值：`TrackLoopLengthCm=360`、`TrackHalfWidthCm=45`。因此 200 cm/s 为约 0.56 圈/秒，360 cm/s 为 1 圈/秒；倒退反转、停止即停、转向产生左右差速，AI 与玩家共用。

验收：UE 5.8.1 Development 完整编译通过；`MineLearning.OreBuddy.SprintAndTracks` 通过（15.15 秒），实测满体力冲刺 2.995 秒，覆盖平滑提速/回落、静止不消耗、耗尽防反复触发、恢复再冲刺、菜单取消，以及同速度不同上限的 UV 一致性、冲刺比例和倒退反转。`MineLearning.Demo.ManualProduction` 通过（44.22 秒）。实际 PIE 中体力值与进度条同为 0.5999556，冲刺色为黄色；切换人类后隐藏，回到 OreBuddy 后重新绑定并显示。
