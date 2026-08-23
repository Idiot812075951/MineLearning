# OreBuddy 采矿与交付

## 职责

OreBuddy 负责“矿石生成端 → 处理机”这一段，不负责搬运金币。分类限制由 `UResourceCarryComponent` 的 `AllowedCategories = Ore` 实现，不在 AI 中判断角色类。

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
| `CollectAnimationPlayRate` | `2.0`，采集动画加倍播放 |
| `DirectMoveSpeed` | `200 cm/s`，窄通道恢复移动速度 |
| `NavigationStallTimeout` | `1.0 s`，无进展后进入本地恢复移动 |

## 碰撞与导航

OreBuddy 胶囊忽略 Pawn，并且不影响 NavMesh；搬运工也使用同样策略，避免两者在机器狭窄通道内面对面锁死。正常路径失败或持续无进展时，AI 会在局部使用确定性的直线恢复移动，到达后仍执行“停止 → 原地旋转 → 交付”。

