# 处理机

## 资产与类

- C++：`AOreProcessorMachine`，继承 `AResourceProcessor`，接收方类型为 `Processor`。
- 蓝图：`/Game/MineLearning/Mining/Processing/Blueprints/BP_ProcesserMachine`。
- 蓝图 EventGraph 当前为空；模型、材质、Spline、SceneComponent 和导航阻挡在 CDO 中配置，运行状态由 C++ 驱动。

## 点位语义：必须区分

正确的设计语义是：

- `InputPoint`：处理机真实入料口，即矿石进入机器运输流程的视觉/逻辑入口。
- 外部 AI 交付点：OreBuddy 可以寻路到达、停车并原地转向卸货的位置，应是另一个独立 SceneComponent，例如 `OreBuddyDeliveryPoint`；它位于入料箱外侧。
- `OutputPoint`：加工完成的金币离开出料履带后停留、等待搬运工拾取的位置。

当前实现存在一项明确的命名债务：`GetDeliveryPointWorldTransform()` 仍把 `InputPoint` 当成 OreBuddy 外部停车点，而视觉入料入口使用 `OreFlowSpline_Input` 的第一个点。当前 BP 中 `InputPoint` 相对位置为 `(85, 70, 17)`。这两个职能不应长期重合；后续调整时应新增独立外部交付组件，再让 `InputPoint` 回归真实入料口。本文档不把当前临时实现误写成最终语义。

`OutputPoint` 当前相对位置为 `(57, -12, 5.3333)`。

## 矿石到金币的流程

1. `CanAcceptItem` 只接受有效的 `IronOre`，并检查输入缓存上限。
2. 接收时按矿石数量生成单件运输 Pickup；每件保持 `Amount = 1`。
3. 每件矿石按 `InputReleaseInterval` 错峰释放，避免一起挤上传送带。
4. 矿石沿 `OreFlowSpline_Input` 运动；队列满时在顶端规则排队。
5. 进入加工口后销毁运输视觉，`QueuedOreCount + 1`，然后开始定时加工。
6. 加工完成后 `QueuedOreCount - 1`，生成 `Coin`。
7. 金币沿 `OreFlowSpline_Output` 运动到 `OutputPoint`。
8. 到达后调用 `ReleaseStationaryForCollection`：金币不再受重力掉落，但可被搬运工拾取。

## 核心配置

| 变量 | 当前值 |
| --- | --- |
| `ProcessingQueueCapacity` | `1` |
| `MaxBufferedInputOre` | `8` |
| `InputOreTravelSpeed` | `35 cm/s` |
| `OutputCoinTravelSpeed` | `14 cm/s` |
| `WaitingOreSpacing` | `12 cm` |
| `InputReleaseInterval` | `0.55 s` |
| `ProcessingTime` | 基类默认 `2.0 s`，可在实例/CDO 配置 |
| `InputOreMeshScale` | `1.0` |
| `CoinMeshScale` | `3.3`，引用共享金币比例 |
| `InputOrePickupClass` | `/Script/MineLearning.ItemPickup` |
| `CoinPickupClass` | `/Game/MineLearning/Mining/Resources/Coin/BP_CoinPickup` |

## 三组视觉表现及其时序

### 传送带与 roller

两者是一组表现，共用同一个条件：输入运输数组中至少有一块矿石尚未到达顶部加工入口。

- 传送带组件：`StaticMesh1` 与 `StaticMesh4`。
- 动态材质参数：`BeltSpeed`；运行值 `-1.35`，停止值 `0`。
- roller Pivot：`RollerPivot_Bottom`、`RollerPivot_Top`。
- 转速：`-220°/s`，按各自局部轴累积旋转。
- 矿石到达顶部并进入等待/加工后，这组表现立即停止；它不依赖处理队列是否仍在加工。

### 能量核心呼吸灯

能量核心独立于传送带：只要 `QueuedOreCount > 0` 就亮。

- 核心组件：`StaticMesh`。
- 动态材质参数：`PulseAmplitude`；工作值 `5.5`，空队列值 `0`。
- 因此可能出现“传送带已停止、核心仍在呼吸”的正确状态，表示矿石已进入队列并正在加工。

运行时只创建动态材质实例并修改上述状态参数；基础材质、静态位置、旋转、可见性和其他常量均由 CDO/资产配置。

## 导航阻挡

当前使用两个细分 Box，而不是整台模型的粗大碰撞轮廓：

| 组件 | RelativeLocation | BoxExtent |
| --- | --- | --- |
| `NavObstacle_Core` | `(0, -10, 30)` | `(30, 28, 32)` |
| `NavObstacle_Core1` | `(45, 30, 15)` | `(17.1545, 13.2505, 13.5538)` |

这样保留入料箱、出料口附近的可达空间。`NavigationBodyMeshComponentName = StaticMesh5` 仅为旧序列化兼容字段，不再是当前导航阻挡真值。

