# MineLearning 玩家 OreBuddy 幻化设计施工单（需求确认版）

> 状态：设计施工单，不是实施记录。
> 核查基线：2026-08-27 当前工作区源码、配置、项目文档与 Unreal 资产只读信息。
> 本轮只更新设计文档，不授权修改代码、蓝图、配置、关卡或资产。

---

## 1. 已确认的第一版需求

### 1.1 幻化规则

1. 场景中只摆放 **一个幻化中心触发器**。
2. 玩家只有位于该触发器范围内时才能幻化。
3. 第一版只支持：

   ```text
   Human ⇄ OreBuddy
   ```

4. 不做形态选择 UI；玩家在触发范围内通过幻化 Input Action 切换 Human/OreBuddy。
5. 第一版没有费用、冷却、持续时间、解锁条件或次数限制。
6. 正确关卡配置下，触发幻化没有正常玩法失败分支。
7. 第一版不尝试 Gunner、Carrier 或其他机器人。

### 1.2 玩家操控与技能

1. 玩家幻化后完整操控 OreBuddy，而不是让 OreBuddy 继续执行 AI 工作。
2. 玩家拥有 OreBuddy 的移动、观察、交互和实际机器人技能行为。
3. 每个技能必须存在 Enhanced Input 映射，玩家可以主动释放。
4. 技能数量不设固定上限，不把程序写死成 Q/E 或两个技能槽。
5. 第一版只绑定 OreBuddy 当前实际存在的动作，不提前制作通用技能栏或万能技能系统。

### 1.3 美术边界

- 幻化中心的建筑、外观、模型、灯光与特效属于美术范围，本施工单不作规定。
- 程序只需要一个可放进场景的 Gameplay Trigger Actor，以及一个明确的安全生成点。
- 以后无论美术把它包装成建筑、平台、机器还是能量场，都不改变幻化核心逻辑。

### 1.4 本版不处理的问题

- UI。
- 其他机器人形态。
- 联网复制、预测与重连。
- 死亡、读档、跨关卡恢复。
- 玩家专属技能和玩家形态数值加成。
- 机器人商店与人口购买流程。

---

## 2. 当前项目事实

| 领域 | 当前事实 | 对施工的影响 |
|---|---|---|
| 玩家身体 | `BP_ThirdPersonCharacter` 继承 `AMineLearningCharacter`；相机、Move/Look/Jump 输入位于 Pawn | Human 输入生命周期需要适配反复 Possess |
| PlayerController | 项目没有自定义玩家控制器；GameMode 只设置 `DefaultPawnClass` | 需要新增一个很小的 PlayerController 作为持久换身事务拥有者 |
| 输入上下文 | Human 只 Add `IMC_Default`，没有成对 Remove | 必须补齐 Add/Remove，避免切换后旧输入仍生效 |
| 玩家采矿 | `IA_Mine` 资产存在，但 Human 当前没有连接的采矿执行入口 | OreBuddy 玩家控制可复用 Input Action 资产，不能假设既有玩家采矿代码可用 |
| OreBuddy 身体 | `AMiningCompanionCharacter` 拥有 `UMiningToolComponent` 和 `UResourceCarryComponent` | 采矿执行与携带数据能够复用 |
| OreBuddy AI | 找目标、寻路、拾取、交付和工作状态主要在 `AMiningCompanionAIController` | PlayerController 接管后要提供玩家手动意图，不能复用 AI 自主状态机 |
| 自动 AI 接管 | `BP_OreBuddy07` 为 `AutoPossessAI = PlacedInWorldOrSpawned` | 玩家形态必须 Deferred Spawn，并在 BeginPlay 前禁用该实例的 Auto Possess AI |
| 相机与输入 | OreBuddy 当前没有玩家相机和玩家输入闭环 | Possess 成功不等于已经可玩 |
| 采矿校验 | `StartMiningTarget()` 自身不校验距离；当前距离条件主要由 AI 流程保证 | AI 与玩家共用前，距离等不变量必须回到共同执行入口 |
| 人口 | `UMiningPlayerData` 有人口 API，但当前没有实际购买调用链 | 玩家形态不调用人口 API；不为此建设商店系统 |
| 幻化中心 | 当前没有对应源码或资产 | 第一版新增一个纯 Gameplay Trigger，不规定美术建筑 |

项目真正需要解决的核心不是“能不能 Possess”，而是：

> **玩家意图与 AI 意图必须调用同一个 OreBuddy 身体和同一套合法能力执行，同时互不接管对方的控制流程。**

---

## 3. 对上一版设计的调整

### 3.1 已被新决定替换的内容

| 上一版内容 | 新决定 |
|---|---|
| 幻化入口待定 | 确认为场景中唯一的幻化中心触发范围 |
| Robot Center/终端/UI 皆未确认 | 只确认 Gameplay Trigger；建筑外观不归程序，第一版无 UI |
| 可能限制携带、动作、费用、冷却、解锁 | 不做玩法费用、冷却、持续时间、解锁或次数限制；动作与携带采用自动收尾，不阻止幻化 |
| Q/E 或 Primary/Secondary 仍待第二个案例决定 | 技能数量明确可变；每个真实技能使用语义 Input Action，不设固定槽数 |
| 后续可能用第二个机器人验证抽象 | 第一版明确只施工 OreBuddy，不接 Gunner/Carrier |
| 切换存在正常失败提示 | 正确配置下不设计正常玩法失败；技术异常保留旧 Pawn 并报告配置问题 |

### 3.2 继续保留的架构结论

- 使用新 Pawn 的 `Spawn + Possess`，不在 Human 上热切全部 Mesh/组件。
- 不占据或修改场景中已有 AI OreBuddy。
- 玩家与 AI 使用同一个 `BP_OreBuddy07` Pawn Class。
- 不复制场景机器人 Runtime Actor。
- 不新增玩家专用 `APlayerOreBuddyCharacter`。
- 不新增 `UPlayerFormComponent`、GAS、技能树或形态 DataAsset 系统。
- PlayerController 只管换身事务，不理解采矿、运输或具体技能。
- 机器人身体拥有自己的相机、玩家输入入口与技能调用。

---

## 4. 推荐的最小结构

```text
场景
└── BP_PlayerFormCenter（唯一实例）
    ├── TransformTrigger：唯一的幻化资格真相
    └── TransformPoint：为 Human/OreBuddy 预留的安全生成点
             │
             │ 玩家在范围内按 IA_Transform
             ↓
AMineLearningPlayerController
└── TogglePlayerFormAt(Center)
    ├── Human → BP_OreBuddy07
    ├── OreBuddy → BP_ThirdPersonCharacter
    └── Deferred Spawn / Possess / 安全兜底

当前 Pawn
├── Human
│   └── Human 相机与输入
└── OreBuddy（AI 与玩家共用同一 Class）
    ├── AIController 控制时：自动工作意图
    ├── PlayerController 控制时：玩家输入意图
    └── MiningTool / Carry：共享的合法能力执行
```

### 4.1 `APlayerFormCenter`

第一版只需要一个很小的 Gameplay Actor：

- 一个 `UBoxComponent` 或其他 `UShapeComponent` 作为触发范围。
- 一个 `USceneComponent` 作为 `TransformPoint`。
- 接收当前玩家发出的幻化请求，并把自己传给 PlayerController。
- 只允许当前 Pawn 确实位于自身 Trigger 内时发起请求。
- 不负责 Spawn、Possess、技能、人口或 UI。

场景只允许摆放一个实例。无需为了“全局唯一”新增 Manager 或 Subsystem；关卡装配和验收负责保证数量为 1。

### 4.2 `AMineLearningPlayerController`

第一版直接由自定义 PlayerController 持有幻化事务，不再套 Component。

最少只需要：

- 一个 `RobotPawnClass`，配置为 `BP_OreBuddy07`。
- 首次从初始 Pawn 记录的 `HumanPawnClass`。
- 一个防止同一帧重复请求的 `bTransformInProgress`。
- 一个公共的 `TogglePlayerFormAt(APlayerFormCenter* Center)`。
- 一个内部的 `SwapToPawnClass()` 事务函数。

不保存 `CurrentForm` 枚举；当前形态由 `GetPawn()` 与 Human/Robot Class 直接判断。

### 4.3 当前 Pawn

身体自己负责：

- 相机。
- Move/Look。
- 该身体可用的 Input Mapping Context。
- 语义明确的技能 Input Action。
- 将玩家操作转成对现有能力的调用。

PlayerController 不写 OreBuddy 类型分支来执行采矿，也不按技能数量循环分发万能槽位。

---

## 5. 幻化中心与请求规则

### 5.1 唯一资格真相

是否允许幻化只由当前 Pawn 与 `TransformTrigger` 的真实空间重叠推导，不在 PlayerController 再维护一份 `bInsideFormCenter` 镜像状态。

```text
IA_Transform
→ 找到当前重叠的 BP_PlayerFormCenter
→ Center 确认当前 Pawn 仍在 Trigger 内
→ PlayerController TogglePlayerFormAt(Center)
```

Trigger 外按幻化键安全无行为；Trigger 内按键必定进入换身流程。

### 5.2 没有 UI

- 进入范围不弹窗。
- 不显示形态列表。
- 不显示 Human/OreBuddy 选择卡。
- `IA_Transform` 在范围内直接切换当前形态。
- 可选的音效、光效或世界提示属于表现，不是核心依赖。

### 5.3 美术与程序解耦

`BP_PlayerFormCenter` 可以只有 Trigger 和 TransformPoint，也可以被美术建筑包裹。美术不得成为是否允许幻化的业务真相；业务只读 Trigger 重叠。

---

## 6. 幻化事务

### 6.1 正常玩法路径

正确关卡配置下，玩家位于 Trigger 内按下幻化键就会成功切换，没有费用、冷却、解锁检查或随机失败。

```text
Center 确认当前 Pawn 在范围内
→ PlayerController 确认当前没有另一笔切换事务
→ 根据当前 Pawn 选择 HumanClass 或 OreBuddyClass
→ 自动收尾当前 Pawn 的瞬时动作/携带状态
→ 在 Center.TransformPoint Deferred Spawn 新 Pawn
→ FinishSpawning 前禁用新实例 AutoPossessAI
→ FinishSpawning
→ Possess(NewPawn)
→ 验证接管成功
→ 销毁 OldPawn
→ 结束事务
```

### 6.2 无限制不等于遗留脏状态

第一版不因动作或携带阻止幻化，但必须在换身前自动收尾：

- OreBuddy 正在采矿：调用现有取消入口，恢复移动与动画状态，然后继续换身。
- OreBuddy 正在携带矿石：把携带物在幻化中心的安全落点重新放回世界，再继续换身。
- Human → OreBuddy：Human 当前没有需要跨身体复制的机器人状态。

这样玩家没有额外限制，也不会因为旧 Pawn 被销毁而悄悄吞掉矿石或遗留 Timer/Delegate。

第一版不建设跨身体 Runtime State Serialization。

### 6.3 “不会失败”的技术定义

产品规则中没有正常失败分支。为兑现这一点：

- Center 提供固定 `TransformPoint`，不在任意拥挤位置盲目生成。
- TransformPoint 的空间必须同时容纳 Human 与 OreBuddy Capsule。
- 关卡验收必须验证 Center、RobotPawnClass、HumanPawnClass 与 TransformPoint 配置完整。
- 连续请求通过 `bTransformInProgress` 合并，不能并发执行两次 Possess。

底层仍保留不可见的安全兜底：如果资产丢失、TransformPoint 被错误堵死或 Possess 出现程序异常，旧 Pawn 不会先被销毁。系统保留旧身体并记录明确错误；这是开发配置保护，不是玩法失败机制。

### 6.4 为什么必须 Deferred Spawn

当前 `BP_OreBuddy07` 会对 Spawn 出来的实例自动生成 AIController。玩家形态必须：

```text
SpawnActorDeferred
→ 对候选实例禁用 AutoPossessAI
→ FinishSpawning
→ PlayerController Possess
```

不能先普通 Spawn 再抢占，否则可能短暂执行 AI BeginPlay、产生孤立 AIController 或领取自动任务。

### 6.5 当前形态的唯一真相

不新增 `EPlayerFormType` 和 `CurrentForm` 字段：

- 当前 Possess Human Class：下一次切 OreBuddy。
- 当前 Possess OreBuddy Class：下一次切 Human。

表现若需要监听换身，可使用既有 Possessed Pawn 变化事件；第一版不额外广播一套重复形态状态。

---

## 7. OreBuddy 玩家控制

### 7.1 同一个 Pawn Class

玩家 OreBuddy 与 AI OreBuddy 都使用 `BP_OreBuddy07`：

- Mesh、Skeleton、AnimBP、Capsule 和移动参数相同。
- `UMiningToolComponent`、`UResourceCarryComponent` 与命中反馈相同。
- 区别由当前 Controller 推导，不保存额外 `bIsPlayerForm`。
- 玩家实例不生成 AIController；场景已有 AI 实例不发生任何变化。

### 7.2 AI 意图与玩家意图分开

```text
AI OreBuddy
AMiningCompanionAIController 选目标/寻路/决定动作
                    ↓
          共享能力与携带执行

Player OreBuddy
相机、移动、瞄准、技能/交互输入
                    ↓
          同一共享能力与携带执行
```

玩家控制时不保留 AIController，也不从 PlayerController 调用 AIController 私有状态机。

### 7.3 相机

OreBuddy 需要与其体型匹配的 SpringArm/Camera。第一版直接在 OreBuddy 身体上补齐最简单；AI 控制实例携带但不使用这些组件。

不要为了只有一个玩家机器人先新增所有机器人共享基类或相机管理器。

### 7.4 “完整操控”的第一版定义

至少包括：

1. OreBuddy 相机、移动和观察。
2. 玩家主动选择合法矿脉并释放采矿技能。
3. 玩家主动拾取 OreBuddy 可携带的 Ore。
4. 玩家主动把携带矿石交给合法接收方。
5. 采矿、携带、分类和接收方规则继续复用现有执行链。
6. 玩家 OreBuddy 不自动找工作、不自动寻路、不自动采矿或交付。

只完成 Possess 与移动仍然只是换身技术验证，不是功能完成。

---

## 8. 可变技能数量与 Enhanced Input

### 8.1 不使用固定技能槽

程序不定义：

```text
Q = Slot 1
E = Slot 2
最多两个技能
```

物理键属于 Input Mapping，技能属于业务语义。技能数量由 OreBuddy 实际绑定的语义 Input Action 数量自然决定。

### 8.2 第一版输入资产建议

| Input Action | 语义 | 使用者 |
|---|---|---|
| `IA_Transform` | 在幻化中心切换 Human/OreBuddy | Human、OreBuddy |
| `IA_Move` | 移动 | Human、OreBuddy |
| `IA_Look` | 观察/瞄准 | Human、OreBuddy |
| `IA_Mine` | OreBuddy 采矿技能 | OreBuddy |
| `IA_Interact` | OreBuddy 手动拾取/交付 | OreBuddy |

第一版可以新增 `IMC_OreBuddyPlayer`：

- 映射 Move/Look/Transform。
- 映射 OreBuddy 当前真实存在的所有技能和交互 Input Action。
- 技能增加时增加语义 Input Action 与映射，不修改一个固定槽位枚举。
- 不做技能栏、动态按键分配 UI 或技能展示数据。

### 8.3 输入生命周期

每个 Pawn 获得本地 PlayerController 时 Add 自己的 Mapping Context，失去时 Remove。Human 与 OreBuddy 都必须成对处理。

连续 Human/OreBuddy 切换后：

- 旧身体的技能不能继续响应。
- 同一次按键只能触发一次。
- Mapping Context 数量不随切换次数增长。

### 8.4 技能执行的共同不变量

Input Action 只表达玩家意图。无论 AI 还是玩家调用，实际能力入口必须统一校验：

- 目标有效。
- 距离合法。
- 必要时视线/交互可达。
- 当前状态允许执行。
- 物品分类与容量合法。
- 接收方确实接受物品。
- 一次动作只提交一次结果。

尤其是现有 `UMiningToolComponent::StartMiningTarget()` 尚未自行校验距离；不能让玩家入口绕过 AI 原有的接近条件。

---

## 9. 第一版施工顺序

> 以下是未来实施顺序，本轮没有执行其中任何一项。

### 第 1 步：建立持久换身拥有者

新增小型 `AMineLearningPlayerController`：

- 记录 Human Class 与 OreBuddy Class。
- 提供 `TogglePlayerFormAt()`。
- 实现 Deferred Spawn、Possess、旧 Pawn 最后销毁与技术兜底。
- 不添加技能分发、人口、UI 或形态数组。

在 `AMineLearningGameMode` 中把 `PlayerControllerClass` 指向它。

### 第 2 步：建立唯一幻化中心

新增 `APlayerFormCenter` 与 `BP_PlayerFormCenter`：

- Trigger。
- TransformPoint。
- 只负责范围确认和转发请求。

在测试关卡中只放置一个实例，并验证生成点能容纳 Human/OreBuddy。

### 第 3 步：接通 Human 幻化输入

- 新增/配置 `IA_Transform`。
- Human 在控制器变化时成对 Add/Remove Mapping Context。
- Trigger 外按键无行为，Trigger 内调用 Center。

### 第 4 步：让 OreBuddy 成为玩家可控身体

- 继续使用 `BP_OreBuddy07`，不建玩家子类。
- 补齐玩家相机、Move、Look。
- 新增/配置 `IMC_OreBuddyPlayer`。
- 接入 `IA_Transform`、`IA_Mine` 与 `IA_Interact`。
- 玩家控制实例不生成 AIController。

### 第 5 步：收敛共享能力规则

- 让采矿共同执行入口校验目标、距离与状态。
- 玩家瞄准只负责选择目标，不直接结算采矿结果。
- 手动拾取/交付继续使用 `UResourceCarryComponent`、`AItemPickup` 与 `IItemReceiver` 的既有规则。
- AI OreBuddy 继续走原 AI 状态机，但调用同一合法执行入口。

### 第 6 步：无条件幻化的自动收尾

- 采矿中切换会先安全取消采矿。
- 携带中切换会把矿石安全放回中心落点。
- 清理与 Pawn 生命周期相关的 Timer、Delegate 和输入上下文。
- 玩家不因这些状态被拒绝幻化。

### 第 7 步：回归与完成验收

- 连续切换。
- Trigger 内外规则。
- 技能输入。
- 采矿距离。
- 携带/交付。
- AI OreBuddy 不受影响。
- 无额外 Pawn、AIController、Mapping Context 或人口变化。

---

## 10. 预计文件与资产影响（施工时）

以下仅用于提前界定未来 Diff，不代表本轮已经创建或修改：

```text
Source/MineLearning/Player/
├── MineLearningPlayerController.h/.cpp     新增
└── PlayerFormCenter.h/.cpp                 新增

Source/MineLearning/
├── MineLearningGameMode.cpp                设置 PlayerControllerClass
├── MineLearningCharacter.h/.cpp            Human 幻化输入与 Mapping 生命周期
├── AI/MiningCompanionCharacter.h/.cpp      OreBuddy 玩家相机/输入/手动意图
└── Mining/MiningToolComponent.h/.cpp       收敛共同采矿合法性

Content/MineLearning/
├── Player/Blueprints/BP_PlayerFormCenter   新增 Gameplay 装配资产
└── Input/
    ├── Actions/IA_Transform                新增
    ├── Actions/IA_Interact                 按现状决定新增或复用
    └── IMC_OreBuddyPlayer                  新增

/Game/ThirdPerson/Maps/ThirdPersonMap       放置唯一 BP_PlayerFormCenter
```

实际动工前仍需重新检查当时工作区与资产引用；不得把本施工单当成无需复核的批量改动命令。

---

## 11. 第一版明确不做

- 不做幻化 UI。
- 不规定幻化中心的美术建筑与表现。
- 不做费用、冷却、持续时间、解锁或次数限制。
- 不做 Gunner、Carrier 或任何第二机器人形态。
- 不做固定 Q/E、固定两槽或技能数量上限。
- 不做通用技能栏、动态键位 UI、GAS、技能树、装备或 Buff。
- 不做 `UPlayerFormComponent`。
- 不做 Form 枚举、形态数组、Data Registry 或多层 DataAsset。
- 不做玩家专用 OreBuddy 子类。
- 不做跨形态 Runtime State Serialization；携带物采用自动落地结算。
- 不做联网、死亡、读档或跨关卡恢复。
- 不做机器人商店与人口系统扩展。
- 不做复杂幻化动画；特效不阻塞可玩闭环。

---

## 12. 后续更强形态的保留方向

本轮不实现，但保持一个边界：

> 玩家与 AI 调用同一能力执行；玩家更强只进入有效参数计算，不复制第二套技能业务。

未来例如玩家 OreBuddy 采矿更强：

```text
基础 MiningPower（OreBuddy 身体配置）
+ 玩家形态加成（持久玩家数据）
= 本次有效 MiningPower
```

加成属于持久玩家数据，不属于会被销毁的 Pawn。当前项目已有 `UMiningGameSubsystem / UMiningPlayerData`，等需求正式进入施工时再决定是否扩展；现在不创建属性、Buff 或技能树系统。

玩家独享技能未来也作为语义 Input Action 对应的真实能力入口接入，不创建 `APlayerOreBuddyCharacter` 复制整套机器人逻辑。

---

## 13. 第一版验收清单

### 幻化中心

- [ ] 测试关卡中恰好存在一个 `BP_PlayerFormCenter`。
- [ ] Trigger 外按 `IA_Transform` 不发生幻化。
- [ ] Human 与 OreBuddy 在 Trigger 内按 `IA_Transform` 能相互切换。
- [ ] 第一版没有幻化 UI。
- [ ] TransformPoint 同时满足 Human 与 OreBuddy 的安全生成空间。
- [ ] 中心的美术外观不会成为业务依赖。

### 换身事务

- [ ] 使用新 Pawn 的 Deferred Spawn + Possess，不切 Mesh。
- [ ] 玩家 OreBuddy 使用与 AI OreBuddy 相同的 `BP_OreBuddy07` Class。
- [ ] 玩家 OreBuddy 在 BeginPlay 前禁用 Auto Possess AI。
- [ ] 场景已有 AI OreBuddy 不被占据、销毁或改 Controller。
- [ ] 正确配置下没有费用、冷却、解锁、随机失败或次数限制。
- [ ] 技术异常时旧 Pawn 不会先被销毁，并记录明确配置错误。
- [ ] 连续切换 20 次没有多余 Pawn 或 AIController。

### 输入与技能

- [ ] Human 与 OreBuddy 都能使用 `IA_Transform`。
- [ ] OreBuddy 有正确的相机、Move 与 Look。
- [ ] OreBuddy 每个实际技能都有语义明确的 Input Action 与映射。
- [ ] 技能数量不受 Q/E 或两个槽位限制。
- [ ] 获得/失去控制时 Mapping Context 成对 Add/Remove。
- [ ] 连续切换后一次按键只触发一次技能。
- [ ] PlayerController 不执行或分发具体 OreBuddy 技能。

### OreBuddy 可玩闭环

- [ ] 玩家可以选择合法矿脉并主动采矿。
- [ ] 过远、无效或不可达目标不会被能力执行入口接受。
- [ ] AI 与玩家调用同一采矿结果链路。
- [ ] 玩家可以手动拾取、携带和交付合法矿石。
- [ ] 采矿中幻化会自动安全取消动作，不拒绝切换。
- [ ] 携带中幻化会把矿石安全放回中心，不丢失资源、不拒绝切换。
- [ ] 玩家 OreBuddy 不自动找工作、寻路、采矿或交付。

### 既有系统保护

- [ ] AI OreBuddy 原有采矿、拾取、交付闭环通过回归。
- [ ] 玩家形态不领取 AI 工作。
- [ ] 玩家形态不调用人口占用或释放。
- [ ] 没有为了首版改造 Gunner、Carrier 或物流系统。

---

## 14. 最终施工结论

第一版只做一个完整而直接的闭环：

```text
唯一幻化中心 Trigger
→ 无 UI 的 IA_Transform
→ Human ⇄ OreBuddy Spawn + Possess
→ 玩家完整操控 OreBuddy
→ 任意数量的语义技能 Input Action
→ 复用现有采矿、携带与交付执行
```

核心结构只增加两个真正必要的 Gameplay 类型：

1. `APlayerFormCenter`：唯一范围入口与安全生成点。
2. `AMineLearningPlayerController`：持久换身事务。

其余工作都落回真实拥有者：相机和输入属于身体，自动工作属于 AIController，采矿与携带规则属于共同执行层。第一版不为 UI、其他机器人或未来强化系统提前增加层级。
