# MineLearning 物流功能链条

当前可玩流程使用 `/Game/MineLearning/Maps/L_WorldLayout_P01`，整局规则与验证见 [矿区复产试运行](../DemoRun.md)。第三人称地图仅保留为测试场景；下面的分项旧文档保留设施结构说明，数值与路线以当前代码、数据资产及本文为准。

## 完整闭环

1. 矿脉受击后生成 `IronOre` Pickup。
2. OreBuddy 只接受 `Ore` 分类，寻找“存在合法接收方”的最近矿石。
3. 自动 OreBuddy 将原矿送回仓库，订单指派 Carrier 提货加工。玩家可携原矿靠近处理机按 E 直接交付，跳过回仓和下单。
4. 处理机把每块矿石沿入料样条送至处理队列；传送带和 roller 只在这段运输期间运动。
5. 两块原矿加工为一块 `IronIngot`，产物沿出料样条到达 `OutputPoint`。
6. 自动 Carrier 默认将铁锭送回仓库，按销售订单送往出售点；玩家 Carrier 可拾取出料铁锭后直接送出售点按 E。
7. 出售点每块铁锭生成两枚金币，仍需 Carrier 运回仓库。Carrier 每次最多携带四件同类货物。
8. 仓库检测搬运单位、开门、更新库存和堆叠显示。只有可用库存能用于购买、升级、解锁和任务交付，预留库存不能重复消费。

## 共同协议

- 数据载体：`FItemStack { ItemType, Amount }`。
- 类型：`IronOre`、`IronIngot`、`Coin`、`Ammo`。
- 分类：`Ore`、`ProcessedMaterial`、`Currency`、`Ammo` 等。
- 接收接口：`IItemReceiver` 提供接收方类型、可接收检查和正式接收三个入口。
- 目的地选择：`UItemLogisticsLibrary::ResolveDestination` 先按数据表优先级，再在同类合法接收方中选择最近者。
- 当前规则资产：`/Game/MineLearning/Mining/Logistics/DT_ItemRules`。

| ItemType | Category | ReceiverPriority |
| --- | --- | --- |
| IronOre | Ore | Warehouse |
| IronIngot | ProcessedMaterial | Warehouse |
| Coin | Currency | Warehouse |
| Ammo | Ammo | Gunner → Warehouse |

加工与销售订单通过显式目的地覆盖默认回仓路线；接收设备忙碌时保留既定路线，不将已提货订单错误地退回仓库。自动 Carrier 优先回收金币，其次执行显式订单，再回收铁锭与散落原矿，避免货款长期滞留出售点。

玩家交互使用 `FindNearbyPlayerMachine`：携带有效货物，在兼容设备的交付点水平 260 cm、垂直 200 cm 内按 E。接收仍走 `IItemReceiver` 的容量和类型检查；忙碌或拒收不清空货物。OreBuddy 保留转向、卸货动画和通知提交。玩家自由拾取不再要求先解析出自动物流目的地。

`MineLearning.Demo.ManualProduction` 验证真实采矿、无订单加工和销售、金币回仓购买，以及自动 OreBuddy 的 4 倍速拾取通知；`PhysicalProduction` 保留仓库订单流程回归。两项同时检查现存和新生成 Pawn 忽略 Camera 碰撞，弹簧臂仍检测环境遮挡。

## 文档索引

- [物品、拾取与通用携带](01_Item_Pickup_And_Carry.md)
- [OreBuddy 采矿与交付](02_OreBuddy.md)
- [处理机](03_OreProcessor.md)
- [搬运工](04_CarrierHauler.md)
- [仓库](05_Warehouse.md)
- [运行资产与关卡装配](06_RuntimeAssets_And_Level.md)

## 统一视觉尺寸

金币的世界显示比例统一由 `MineLearningItemVisual::GoldCoinScale` 控制，当前为 `3.3`。处理机出料、搬运工箱内金币和仓库库存展示都引用同一常量，避免在多个环节分别缩放。
