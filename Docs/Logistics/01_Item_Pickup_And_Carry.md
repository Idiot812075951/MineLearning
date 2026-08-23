# 物品、拾取与通用携带

## 运行流程

`AMineableOre` 仍按矿石定义和掉落规则生成资源，但实际世界物品统一进入 `AItemPickup`。Pickup 保存完整 `FItemStack`，负责预订、物理状态、运输锁定和收集；收集成功后把同一个 `ItemType / Amount` 交给角色的 `UResourceCarryComponent`。

携带组件依据数据表中的 `Category` 判断是否接受，而不是根据角色类型写特判。组件只能同时保存一种物品类型；增加、取出和清空都会广播 `OnCarryChanged` 并刷新携带预览。

## 核心 C++

- `ItemTypes.h`：物品类型、分类、接收方类型、`FItemStack` 和共享金币比例。
- `ItemRules.h`：`FItemRuleRow` 数据表行结构。
- `ItemReceiver.h`：所有处理机、仓库等接收方实现的接口。
- `ItemLogisticsLibrary.*`：读取 `DT_ItemRules` 并解析合法目的地。
- `ItemPickup.*`：通用世界 Pickup。
- `ResourcePickup.*`：旧矿石 Pickup 的兼容子类；真实存储数据已收敛到 `FItemStack`。
- `ResourceCarryComponent.*`：容量、分类过滤、携带数据和预览。
- `ResourceStorageComponent.*`：按 `EItemType` 存储数量；旧矿石预留 API 继续兼容。

## Pickup 状态

- 普通掉落：物理、重力开启；对 Pawn 不阻挡，也不影响导航。
- `TryReserve`：防止两个工人同时选择同一件物品。
- `SetTransportLocked(true)`：处理机样条运输期间关闭物理与外部收集。
- `ReleaseStationaryForCollection`：金币到达出料点后保持静止，不再掉到机器下面，但重新允许收集。
- `TryCollect`：由携带组件决定可添加数量；完全取走后销毁 Pickup。

## Carry 核心配置

| 配置 | 作用 |
| --- | --- |
| `Capacity` | 最大携带数量 |
| `bAcceptAllCategories` | 是否接受所有分类 |
| `AllowedCategories` | 分类白名单 |
| `PreviewSocketName` | 默认 `S_CargoBin`，用于旧矿石预览 |
| `PreviewResourceTransforms` | 多件物品在容器中的预览位置 |

当前角色策略：

- OreBuddy：容量 `4`，只接受 `Ore`。
- Carrier Hauler：容量 `1`，只接受 `Currency`。

## 存储

`UResourceStorageComponent` 使用 `TMap<EItemType, int32>` 保存各物品数量，`MaxItemCapacity` 默认 `100`，设为 `0` 表示无限。`StoredOreCount` 已不再是独立真值；旧接口从 `IronOre` 条目派生，避免维护两份库存。

