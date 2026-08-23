# 运行资产与关卡装配

## 当前正式运行资产

### 物流规则

- `/Game/MineLearning/Mining/Logistics/DT_ItemRules`
- `/Game/MineLearning/Mining/Logistics/Blueprints/BP_Hauler`

### 搬运工

- `/Game/MineLearning/Characters/Carrier/SK_CarrierRobot`
- `/Game/MineLearning/Characters/Carrier/SK_CarrierRobot_Skeleton`
- `/Game/MineLearning/Characters/Carrier/ABP_CarrierRobot`
- 五个 `CarrierRobotRIG_CarrierRobot_AN_Carrier_*` 动画序列
- `SM_Carrier_CargoBox` 与少量 Carrier 材质

### 处理机

- `/Game/MineLearning/Mining/Processing/Blueprints/BP_ProcesserMachine`
- `/Game/MineLearning/Mining/Processing/Resource/SM_OreProcessor_Body`
- `M_OP_ConveyorBelt_Animated`
- `M_OP_CorePulse_Animated`
- 其余 `M_OP_*` 正式基础材质和导入的处理机资源

### 金币

- `/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin`
- `/Game/MineLearning/Mining/Resources/Coin/M_GoldCoin`
- `/Game/MineLearning/Mining/Resources/Coin/BP_CoinPickup`

### 仓库

- `/Game/MineLearning/Mining/Storage/Blueprints/BP_Warehouse`
- 已有 Warehouse Mesh 与 Material 资源

## 最终源文件

- 搬运工：`ArtSource/Characters/CarrierRobot/CarrierRobot_V08_ArmPosePolish.blend`、`CarrierRobot.fbx`。
- 金币：`ArtSource/Items/GoldCoin/SM_GoldCoin.blend`、`SM_GoldCoin.fbx`。
- 处理机：`ArtSource/OreProcessor/OreProcessor_V26_InclineBeltCleanSlab.blend`；当前结构化导入源 `OreProcessor_V21_UEAssetStructure.fbx`。
- 仓库：`ArtSource/Warehouse/Warehouse_V06_UEExportReady.blend`、`SM_Warehouse.fbx`。

## ThirdPersonMap 装配

当前关卡存在以下正式实例：

- `BP_OreBuddy07`
- `BP_ProcesserMachine`
- `BP_Hauler`
- `BP_Warehouse`

World Partition/External Actors 的未提交变动包含上述实例的替换、位置或配置保存。提交时必须把 `Content/__ExternalActors__/ThirdPerson/Maps/ThirdPersonMap` 和对应 `__ExternalObjects__` 变化与资产、C++ 一起提交，否则其他工作区可能只得到代码和蓝图，却看不到正确关卡实例。

## 兼容资产

- `BP_ResourceProcessor`：旧通用处理机资产；基类现已实现 `IItemReceiver`，但真实可视流程由 `BP_ProcesserMachine` 承担。
- `BP_ResourceStorage`：旧通用存储资产；继续使用升级后的 `UResourceStorageComponent`。
- `AResourcePickup`：旧矿石类名兼容层；真实 Pickup 行为在 `AItemPickup`。

## 维护边界

- CDO/资产负责：静态 Mesh、基础材质、组件相对 Transform、Spline、导航 Box 和动画资产引用。
- C++ 负责：物品状态、AI 状态机、处理队列、事件驱动视觉参数、门逻辑和库存。
- 蓝图 EventGraph 不再承担 Debug 自动演示或大量运行时赋值。
- 处理机点位后续调整时，必须先拆分“真实 `InputPoint` 入料口”和“箱外 OreBuddy 交付/停车点”，不要再次混用同一组件。

