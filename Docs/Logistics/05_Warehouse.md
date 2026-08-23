# 仓库

## 资产与类

- C++：`AWarehouseDepot`，继承 `AResourceDepot`，接收方类型为 `Warehouse`。
- 蓝图：`/Game/MineLearning/Mining/Storage/Blueprints/BP_Warehouse`。
- 存储：`UResourceStorageComponent`。

仓库业务已从大型 Debug EventGraph 收敛到 C++。蓝图只保留美术组件和一个 Construction Script：把 `WarehouseForwardRoot` 旋转到项目统一朝向；没有运行时 Debug 工人、Debug Cargo 或自动循环节点。

## 运行流程

1. 搬运工通过 `P_Warehouse_DockPoint` 获取交付 Transform。
2. 真实 `AHaulerCharacter` 进入 `WarehouseWorkerTrigger` 后请求开门。
3. `DoorPivot` 以恒定角速度从 `0°` 转到 `DoorOpenRoll`。
4. 门未完全打开时，`BarrierVisual` 保持可见，`BC_Warehouse_DoorSafetyBlocker` 保持碰撞。
5. 门完全打开后，激光门帘隐藏，安全阻挡关闭。
6. `AcceptItem` 把金币写入 Storage，刷新仓内金币堆叠，并安排关门。
7. 若搬运工仍在触发区，关门计时会延后；离开后再关闭。

## 核心配置

| 变量 | 当前值 |
| --- | --- |
| `DoorRotationSpeed` | `180°/s` |
| `DoorOpenRoll` | `90°` |
| `MaxVisibleCoins` | `12` |
| `MaxItemCapacity` | Storage 默认 `100` |

## 关键 SceneComponent

以下为蓝图相对 Transform 中的位置：

| 组件 | RelativeLocation | 用途 |
| --- | --- | --- |
| `P_Warehouse_DockPoint` | `(0, 195, 115)` | 搬运工正式交付 Transform；朝向 Yaw `180°` |
| `P_Warehouse_WaitPoint` | `(0, 237, 26)` | 预留等待/编队点；当前核心 AI 不依赖它 |
| `P_Warehouse_CargoPoint` | `(0, 190, 42)` | 门外托盘货物参考点 |
| `P_Warehouse_CargoInside` | `(0, 25, 70)` | 仓内库存金币展示根节点 |
| `DoorPivot` | 美术配置 | 下沿铰链开门 Pivot |
| `BarrierVisual` | 美术配置 | 激光门帘 |
| `BC_Warehouse_DoorSafetyBlocker` | 美术配置 | 门未完全开启时的安全碰撞 |

## 库存视觉

仓库使用 `WarehouseInventoryCoins` Instanced Static Mesh 展示最多 12 枚金币。实例按 3 列、2 行、多层排列，每个实例使用共享金币比例 `3.3`。实际库存保存在 `UResourceStorageComponent::StoredItems`；视觉数量只是上限裁剪，不是业务真值。

