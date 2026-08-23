# 搬运工

## 职责

搬运工负责“处理机金币出料点 → 仓库”这一段。它不是玩家角色，也不接 OreBuddy 的矿石工作；`UResourceCarryComponent` 配置为容量 `1`、只接受 `Currency`。

## 运行资产

- Actor 蓝图：`/Game/MineLearning/Mining/Logistics/Blueprints/BP_Hauler`。
- Skeletal Mesh：`/Game/MineLearning/Characters/Carrier/SK_CarrierRobot`。
- Animation Blueprint：`/Game/MineLearning/Characters/Carrier/ABP_CarrierRobot`。
- Cargo Box：`/Game/MineLearning/Characters/Carrier/SM_Carrier_CargoBox`。
- AI Controller：`AHaulerAIController`。
- AnimInstance C++：`UCarrierAnimInstance`。

`BP_Hauler` 的 EventGraph 只有未连接的默认事件入口，没有物流业务节点；主要配置来自 C++ CDO。`AIControllerClass` 为 `HaulerAIController`，`AutoPossessAI = PlacedInWorldOrSpawned`。

## AI 状态机

`Idle → MovingToPickup → PickingUp → MovingToDestination → DroppingOff → Idle`

1. 每 `0.4 s` 搜索最近可接受 Pickup。
2. 搜索时先调用 `ResolveDestination`；没有合法仓库就不拾取。
3. 预订金币并 MoveTo；到达后播放 PickUp 动画。
4. `Pickup` Notify 执行真实收集，并显示 `S_Cargo` 上的 CargoBox 和箱内金币。
5. 解析仓库并移动至仓库提供的 DockPoint。
6. 播放 DropOff 动画；`DropOff` Notify 调用仓库 `AcceptItem`。
7. 成功后清空携带、隐藏箱子和金币、解除目标，重新搜索任务。

## 关键 AI 变量

| 变量 | 当前值 |
| --- | --- |
| `SearchInterval` | `0.4 s` |
| `PickupAcceptanceRadius` | `90 cm` |
| `DestinationAcceptanceRadius` | `160 cm` |
| `DirectMoveSpeed` | `320 cm/s` |
| `NavigationStallTimeout` | `1.0 s` |

搬运工胶囊不阻挡 Pawn、不影响 NavMesh。正常 MoveTo 失败或卡住时会使用局部直线恢复移动，避免在处理机出料口和 OreBuddy 互堵。

## 动画对接

- 空手待机：`AN_Carrier_Idle`。
- 空手行走：`AN_Carrier_Walk`。
- 拾取：`AN_Carrier_PickUp`，通过 `PlaySlotAnimationAsDynamicMontage` 播放。
- 携货行走：`AN_Carrier_CarryWalk`。
- 交付：`AN_Carrier_DropOff`，同样动态 Montage 播放。

`UCarrierAnimInstance` 每帧写入：

- `GroundSpeed`：取 CharacterMovement 速度与实际位移速度的较大值，兼容直线恢复移动。
- `bIsMoving = GroundSpeed > 3`：速度为零时进入 Idle，不再原地走腿。
- `bHasCargo`：携带组件非空时为真，用于选择 CarryWalk。

动画 Notify 是正式提交点；角色另有基于动画时长的计时器兜底，避免 Notify 丢失导致状态机永久卡住。

## 箱内金币

CargoBox 附加到骨骼 Socket `S_Cargo`。`CargoContentVisual` 是箱子的子组件，位于相对 `(0, 0, 10)`，金币比例使用共享 `3.3`；碰撞、Overlap 和导航影响全部关闭。

