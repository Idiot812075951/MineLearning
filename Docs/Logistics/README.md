# MineLearning 物流功能链条

本文档组记录当前未提交版本中的完整物流闭环、主要运行资产、C++ 职责和 UE 配置。当前测试关卡为 `/Game/ThirdPerson/Maps/ThirdPersonMap`，关卡内各放置了一台 OreBuddy、处理机、搬运工和仓库。

## 完整闭环

1. 矿脉受击后生成 `IronOre` Pickup。
2. OreBuddy 只接受 `Ore` 分类，寻找“存在合法接收方”的最近矿石。
3. OreBuddy 抵达处理机交付位置后停止移动、原地转向，再播放卸货动作。
4. 处理机把每块矿石沿入料样条送至处理队列；传送带和 roller 只在这段运输期间运动。
5. 处理队列非空时，能量核心独立显示呼吸灯；每批加工完成后生成 `Coin`。
6. 金币沿出料样条到达 `OutputPoint`，保持静止并可被搬运工拾取。
7. 搬运工只接受 `Currency` 分类，携带一个金币前往仓库。
8. 仓库检测真实搬运工、开门、关闭安全门帘与阻挡；金币入库后刷新内部堆叠显示，再安全关门。

## 共同协议

- 数据载体：`FItemStack { ItemType, Amount }`。
- 类型：`IronOre`、`Coin`、`Ammo`。
- 分类：`Ore`、`Currency`、`Ammo` 等。
- 接收接口：`IItemReceiver` 提供接收方类型、可接收检查和正式接收三个入口。
- 目的地选择：`UItemLogisticsLibrary::ResolveDestination` 先按数据表优先级，再在同类合法接收方中选择最近者。
- 当前规则资产：`/Game/MineLearning/Mining/Logistics/DT_ItemRules`。

| ItemType | Category | ReceiverPriority |
| --- | --- | --- |
| IronOre | Ore | Processor → Warehouse |
| Coin | Currency | Warehouse |
| Ammo | Ammo | Gunner → Warehouse |

## 文档索引

- [物品、拾取与通用携带](01_Item_Pickup_And_Carry.md)
- [OreBuddy 采矿与交付](02_OreBuddy.md)
- [处理机](03_OreProcessor.md)
- [搬运工](04_CarrierHauler.md)
- [仓库](05_Warehouse.md)
- [运行资产与关卡装配](06_RuntimeAssets_And_Level.md)

## 统一视觉尺寸

金币的世界显示比例统一由 `MineLearningItemVisual::GoldCoinScale` 控制，当前为 `3.3`。处理机出料、搬运工箱内金币和仓库库存展示都引用同一常量，避免在多个环节分别缩放。

