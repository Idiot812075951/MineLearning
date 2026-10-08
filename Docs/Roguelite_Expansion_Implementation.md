# 肉鸽内容扩充实现记录

日期：2026-10-03。

## 后续修正：镜头、近战与钻头充能

- `MineLearningPlayerController.*` 统一在 `AddYawInput/AddPitchInput` 检查右键按住状态。常态不锁鼠标，按键期间临时捕获保证左键第一下正常开火；菜单打开仍显示鼠标并阻止镜头输入。Gunner 右键短按在松开时切换三连发，拖动镜头不切换。
- `Presentation/CombatFeedbackComponent.*` 订阅 `Combat.OnAttributesChanged`，仅在实际攻击距离改变时立即显示红圈 2 秒；重复变化重新计时，超距攻击也继续使用同一红圈。
- `Combat/CombatComponent.*::FindNearestAttackTarget` 统一筛选当前射程内、允许受伤、无遮挡的最近目标。OreBuddy 出手和红莲普攻出手/各段命中使用该查询，只旋转 Actor，不修改镜头 ControlRotation；无目标沿当前朝向空释放。搬运工继续使用货物交互规则，拾取时朝向货物。
- `Mining/MiningToolComponent.*` 的目标扩展为 `AActor`，OreBuddy 可钻采矿石或攻击敌方单位；有效性继续交给统一伤害规则，保留完整多段动作与空挥能力。移除旧的未使用矿石专属搜索和距离计算。
- `Combat/ChargedMiningEffect.cpp` 不再发布全身光环，只发布工具充能状态。表现组件通过可配置 `ToolMaterialSlot`（默认 `MAT_OB07_ToolMetal`）改变该槽的动态材质。`/Game/MineLearning/Characters/OreBuddy/Materials/M_OB07_ToolMetal` 新增 `ChargeIntensity`（默认 0）与 `ChargeColor`，在原材质发光上叠加充能光，其他身体槽保持原样。强化期间全程发光，动作结束/效果撤销归零。
- 验证包含完整 Development Editor 编译、OreBuddy 蓝图编译、`CombatFeedback`（射程变化/空挥/转向/局部材质）、`ChargedWholeCycle`、`UIFlow`（右键按住/松开、菜单与鼠标策略）、`PIEIntegration`（开火、攻速与三连发切换）。
- 最终结果：以上 4 项针对性测试全部通过，0 失败；完整编译与 OreBuddy 蓝图编译通过。测试地图的既有 RecastNavMesh 启动警告仍会出现。

**当前状态：本轮代码、11 张卡牌配置、4 个永久节点、2 个召唤师、输入及 UMG 已实装。编译通过；最终运行验收记录见本文末尾。**

依据：用户提供的 `Downloads/Roguelite_New_Content_Work_Order_2026-10-03.md`，以及本轮补充。现有已提交肉鸽内容不在本文件中重新统计。

## 本轮覆盖规则

- 红莲不新增卡牌，只迁移轮选键位。
- 蓄能钻头强化整次钻采：一次动作开始时锁定是否具备充能，首次真实命中才消耗；该动作后续全部伤害段均 ×2，光效维持至动作结束。空挥保留充能；中途充满留给下次动作；强化动作结束才开始下一轮充能。
- 巨人杀手、歌利亚巨人按用户修订变为玩家当前幻化单位专享，不自动强化 AI / 幻影。这里覆盖施工单原来的 Carrier 专属说明及两个 AI 巨人组合例子。
- 货舱百分比在同一层相加，最后向上取整。失去容量不删已有货物。
- 超频状态由玩家控制器上的组件持有，换 Pawn 不刷新阶段与冷却。
- 主线程按团队 +25%、主力额外 +75%，不是 +125%；采矿/射击单位解释为普攻伤害，搬运单位解释为容量，不加三速。

## 代码位置与职责

| 文件 | 本轮职责 |
| --- | --- |
| `Combat/CombatModifiers.*`、`Combat/WeaponRecoilComponent.cpp` | BulletDrift 百分比同时作用于既有弹道轨迹与随机偏移；不改变伤害/射程 |
| `Mining/ResourceCarryComponent.*` | 按来源管理容量贡献；真实 Carry→Carry 交接先提交双方数据，再广播；接收方满载/不接受时不扣来源 |
| `Mining/ItemPickup.cpp` | 付费物流订单改为查询货舱接收能力，不再硬编码只允许 Hauler 类；原预留库存提交仍复用 |
| `Combat/UnitEffectComponent.*` | 将效果的容量贡献交给 Carry；将体型、颜色、地面环、工具发光等展示信息发布给表现层 |
| `Mining/MiningToolComponent.*` | 发布周期开始/结束、命中准备、真实命中提交；不识别卡牌 ID，不创建效果实例 |
| `Combat/ChargedMiningEffect.*` | 可配置 8 秒充能、命中缩短 0.75 秒、整次 ×2、Impact ×1.5；计时与动作钩子成对清理 |
| `AI/AutonomousUnit.h` | 单位声明自身工作输出类别；没有声明的单位不接受主线程输出强化 |
| `Roguelite/RunAbilityComponent.*` | 超频/过热/冷却、换形接续、己方 AI 强化、稳定注册顺序轮选主力、死亡与销毁重选 |
| `AI/HaulerAIController.*` | 协作任务对空闲搬运工进行独占 Claim；正常已开始的物流工作不被抢占 |
| `AI/CooperativeHaulingComponent.*` | 在已注册己方搬运工中分配接力/双人任务；接力范围、成功加速、超时均来自配置 |
| `AI/SharedCarryTask.*` | 两名参与者与一份共享货舱；真实提货和交付；成员失效时向幸存者交接，余货恢复为合法拾取物 |
| `AI/CarrierAnimInstance.*` | 向动画蓝图提供 `bSharedCarry`，驱动独立抬箱分支；原行走序列继续提供腿部动作 |
| `Presentation/CombatFeedbackComponent.*` | 订阅效果状态，处理视觉 Mesh 缩放、光环/标记/工具灯；不修改碰撞、导航或伤害 |
| `Roguelite/RunContentCatalog.*` | 新能力配置引用、身份幻化许可、初始生产班组字段及配置验证 |
| `Roguelite/MineRunCoordinatorComponent.cpp` | 根据本局已购卡牌装配能力；只在应用层知道天赋、身份和卡池；换控制者时撤销旧单位的主角专属效果 |
| `MineLearningPlayerController.*`、`PlayerTransformZone.cpp` | 控制器通过通用幻化许可接口阻止非法换形；T 召唤师能力、X 上下文目标、Y 天赋、Tab 中控台 |
| `Demo/DemoRunComponent.*` | 身份初始班组配置支持；走既有招聘交易，失败时未花掉的启动资金仍保留；修正 Q 钻采旧提示 |

上述路径均相对于 `Source/MineLearning/`。

底层攻击、货舱与接收方不读取召唤师名字、天赋节点或卡牌 ID。应用装配层将已拥有能力连接到这些业务模块；表现层消费结果。

## 已创建的 UE 配置

配置目标仍为现有 UE DataTable / DataAsset，不使用 INI。

| 卡牌 | 核心配置 | 适用范围 |
| --- | --- | --- |
| 长轴钻头 | AttackRange +0.5、PrimaryDamage -0.1 | 所有己方 OreBuddy |
| 暴力钻头 | PrimaryDamage +0.45、AttackSpeed -0.2 | 所有己方 OreBuddy |
| 蓄能钻头 | `UChargedMiningEffectDefinition`，8 / 0.75 / ×2 / ×1.5 | 所有己方 OreBuddy |
| 巨人杀手 | VisualScaleBonus -0.3、MoveSpeed +0.35 | 玩家当前幻化单位 |
| 歌利亚巨人 | VisualScaleBonus +0.35、CarryCapacityPercent +1、MoveSpeed -0.15 | 玩家当前幻化单位；容量仅在有货舱时有意义 |
| 接力棒 | `URelayHaulingDefinition`，700 cm，+60% 移速 5 秒 | 玩家携货与己方空闲 Carrier AI |
| 协力重载 | `USharedCarryDefinition`，容量和 ×1.5，速度最小值 ×0.9，需求阈值容量和 ×0.75 | 两名己方空闲 Carrier AI |
| 加长枪管 | AttackRange +0.7、BulletDrift -0.6、AttackSpeed -0.2 | 所有己方 Gunner |
| 短管突击 | AttackRange -0.35、PrimaryDamage +0.3、AttackSpeed +0.25、BulletDrift +0.3 | 所有己方 Gunner |
| 超频 | `UOverclockDefinition`，三速 +100% 6 秒、-20% 3 秒、冷却 20 秒 | 超频者付费专属；玩家当前单位 |
| 主线程 | `UAIWorkDefinition`，BasePower 0.25、FocusPower 0.75 | 面向同事编程付费专属；支持工作输出的己方 AI |

新增永久节点：`MiningMods`（矿务改装）、`LogisticsMods`（物流改装）、`Overclocker`（超频者资格）、`CoworkerProgramming`（面向同事编程资格），各 1 点。资格只解锁卡池/身份，能力仍需局内付费三选一。枪管卡要求本局已购 Gunner。没有给红莲新增卡牌。

### 在 UE 中修改配置

均位于 `/Game/MineLearning/GameplayRuntime/`：

- `Data/DT_Upgrades`：本轮新增 11 行，当前共 23 行。管理名称、短描述、完整描述、图标、目标范围、永久资格、召唤师限制及效果/能力引用。
- `Data/DT_Talents`：本轮新增 4 行，当前共 11 行。管理节点价格、解锁项和位置。
- `Data/DT_Summoners`：新增 2 行，当前共 4 个身份；另有 UI 提供的免费默认身份。“面向同事编程”关闭幻化权限，初始配置为 1 台 OreBuddy + 1 台 Carrier，保障新档能开工；不免费赠送主线程。
- `Effects/DA_<ID>`：下表每张卡有同名 DA。普通数值使用既有 `UUnitEffectDefinition`；充能、接力、共享搬运、超频、主线程使用各自最小能力配置。没有 INI。

| ID（也用于 GM） | 展示名 | DA 特殊字段 |
| --- | --- | --- |
| LongDrill | 长轴钻头 | Rule.Modifiers |
| HeavyDrill | 暴力钻头 | Rule.Modifiers |
| ChargedDrill | 蓄能钻头 | ChargeSeconds / HitReduction / DamageMultiplier / ImpactScale |
| GiantSlayerCarrier | 巨人杀手 | Rule.VisualScaleBonus / Modifiers.MoveSpeed；DT Audience=ControlledPlayer |
| GoliathCarrier | 歌利亚巨人 | Rule.VisualScaleBonus / CarryCapacityPercent；DT Audience=ControlledPlayer |
| RelayBaton | 接力棒 | SearchRadius / Timeout / HandoffBoost |
| CoopHeavyCarry | 协力重载 | TaskClass / CapacityMultiplier / MinimumLoadFraction / MoveSpeedMultiplier / HalfSpacing / SearchRadius / StallTimeout / DispatchBatchSize |
| LongBarrel | 加长枪管 | Rule.Modifiers.AttackRange / BulletDrift / AttackSpeed |
| ShortBarrelAssault | 短管突击 | Rule.Modifiers.AttackRange / PrimaryDamage / AttackSpeed / BulletDrift |
| Overclock | 超频 | Boost / Overheat / Cooldown |
| MainThread | 主线程 | DisplayRule / BasePower / FocusPower |

`GiantSlayerCarrier`、`GoliathCarrier` 的内部 ID 保留施工单名称，但实际范围已经按用户要求改为当前玩家幻化单位，不是 Carrier 专属，也不强化 AI。Mesh 缩放与容量是不同业务；没有货舱的单位只获得有意义的部分。

### 调用链与分层

1. 天赋存档给出本局资格快照；身份免费选择。
2. 商店按现有 4、8、12…金币购买三选一；`UpgradeDraftComponent` 生成合法候选，选择交给 `RunBuildComponent`。
3. `MineRunCoordinatorComponent::BuildChanged` 从已拥有的行读取能力引用，装配到单位或玩家级模块。下层不读取天赋名或召唤师名。
4. 普通数值：`UnitEffectComponent` → `CombatComponent` / `ResourceCarryComponent`。展示状态发布给 `CombatFeedbackComponent` 与 UMG。
5. 充能：`MiningToolComponent` 发布动作和命中事件 → `ChargedMiningEffectInstance` 锁定整次强化 → 命中上下文携带倍率 → 原伤害流程执行。结束事件统一卸下本次强化，不按第一段命中撤销。
6. 召唤师：控制器持有 `RunAbilityComponent`。超频计时不属于 Pawn；主线程遍历装配层注册的己方 AI，通过 `IAutonomousUnit` 声明的输出类型施加强化。
7. 接力/双人：`CooperativeHaulingComponent` 只调度已注册己方空闲搬运工；使用 `HaulerAIController` 的任务占用接口。接力使用真实 `Carry.TransferTo`，成功后才加速。
8. 双人共享任务是唯一货物所有者。先占用两个人和一份拾取物，靠近后提交一次预留库存；配送完成仅入库一次。一人死亡、销毁、被控制或路径受阻时，释放任务，向有效幸存者交接，余量回落为拾取物。
9. 仓库保持库存权威。装配层在取得协力卡后把**新订单**分包上限改为 DA 配置的 12；已有订单不重写，普通搬运工仍可部分领取。仓库不知道肉鸽卡牌或身份。

### 双人寻路与临时表现

`SharedCarryTask` 为集合和交付分别请求导航路径，按路径点推进队形；移动保留 Sweep，台阶处理复用 CharacterMovement。形成队形前按目的地方向对齐，不让起点已就位的零长度路径判为失败。队形宽度超过普通单人，因此极窄通道或未写入导航的动态障碍仍可能触发 3 秒阻挡恢复。这是当前临时实现的明确边界，不是保证任何地形都可双人通行；不会穿墙强送。

实际资产：

- `World/BP_SharedCarryTask`：蓝灰箱体、框架、双侧把手、青色灯条；通过 `OnTaskChanged` 只在 Delivering 显示。货物结算不依赖箱子 Mesh。
- `/Game/MineLearning/Characters/Carrier/ABP_CarrierRobot`：新增 `bSharedCarry` 姿态分支，在既有 Carry 动作上作上臂调整；保留原 Skeleton、Rest Pose、拾取/放下和行走序列。结束/中断回到普通分支。
- `FX/M_CoopCargoShell`、`M_CoopCargoFrame`、`M_CoopCargoLight`：临时箱体材质。
- `FX/M_WorkRing`、`M_WorkMarker`：团队地面环和主力标记；新主力触发短脉冲。
- `FX/M_OverheatSteam`：少量上升的半透明蒸汽；由通用效果的 `bSteam` 展示字段驱动。
- `M_StackAura` 新增可配置 AuraColor；充能钻头保留工具点光和脉冲，超频橙色，过热低强度灰色。充能音效及超频开启音效暂时复用项目已有 Guren 音频，分别调整音量/音高；以后可直接替换配置。

表现层只读取状态，不改伤害、容量、碰撞或卡池。`ActivationSound` 对同一来源的同一音效只在进入时播放，持续时间刷新不会重复播放。

### UI、键位与图标

- `WBP_RogueliteHub` 新增 AbilityStatus，由 `RunAbilityComponent.OnAbilityChanged` 事件驱动；Construct 绑定后首刷，Destruct 解绑，不使用 Widget Tick。
- 新增召唤师水平滚动和天赋页垂直滚动，避免新增卡片/节点越出面板。
- T：超频；X：主力 AI 轮选／红莲上下文目标；Y：天赋。Tab 保持原生产中控台。
- 输入资产：`/Game/MineLearning/Input/Actions/IA_SummonerAbility`、`IA_CycleContextTarget`、`IMC_RunAbilities`。保留原生键位降级路径。
- 新增 11 张独立卡牌源图，保存在 `ArtSource/UI/RogueliteExpansion/`；UE 纹理 `UI/Art/T_Expansion_<ID>`，材质实例 `MI_Icon_<ID>`，运行时最大尺寸 512。卡牌、效果、天赋与召唤师引用相应图标。
- 图标通过内置 imagegen 独立生成；最终提示及语义记录在 `ArtBriefs/RogueliteUI.md`。没有覆盖旧图标。

## 快速试玩

1. Y 打开天赋，用现有 GM 天赋点按钮研究“矿务改装”“物流改装”或召唤师资格，再开局。局内不能更换已锁定身份。
2. 正常体验：靠近升级商店花金币三选一。天赋和身份提供出现资格，不免费发卡。
3. 快速验收：开局后控制台执行 `UpgradeAdd ChargedDrill` 等上表 ID。此命令直接走本局升级获取接口，仍检查天赋、身份、形态购买和最大等级；它是明确的 GM 免付费入口，不是普通玩家规则。
4. OreBuddy 蓄能：等待钻头发亮后采矿，整个动作的多段都强化；空挥不消耗。长轴/暴力可同时叠加。
5. 超频者：先取得 Overclock，再按 T；换形不会重置 20 秒冷却。
6. 面向同事编程：免费选择身份后以人类开局，自带普通生产班组；取得 MainThread 后按 X 轮选主力。不能幻化，HUD 引导改为生产调度。
7. 接力：取得 RelayBaton，玩家持有可接收货物，附近有空闲 Carrier；它靠近后真实交接并加速。
8. 双人：取得 CoopHeavyCarry，准备两台空闲 Carrier，在仓库管理面板下达至少 6 个的单笔订单（12 个可装满默认共享箱）。原来 Tab 的加工 4 个快捷按钮仍是小单，不会强行凑成大箱。运输发生阻挡时会安全回落到正常物流。

## 验证与边界

最终结果：本轮 5 项专项测试 + 最后 10 项回归全部通过，共 15 项；另有主地图 NavigationContract 通过。测试使用独立自动化存档，不清空玩家原存档。最后 10 项批量调用超过工具的 60 秒等待期限，但编辑器继续执行；随后通过 GetTestResults 读取确认 10 成功、0 失败（69.36 秒），没有把超时当成测试通过。

专项：CapacityAndTransfer、ChargedWholeCycle、SharedCargoDelivery、SharedCargoRecovery、PIEHauling。PIEHauling 覆盖实际调度组件自动生成箱子、进入抬箱动画、工业区队形交付，以及玩家→AI 的真实接力与成功加速。

回归：PIEAbilities、ActionSpeed、AttackStacks、Catalog、CombatFeedback、PaidDraft、TalentPersistence、UIFlow、WeaponEffects、Warehouse.ReservationContract。初始化/空白测试地图存在既有 RecastNavMesh 提示；独立主地图导航检查成功。

- C++：UE 5.8 `MineLearningEditor Win64 Development` 完整编译和链接通过。
- Blueprint：WBP_RogueliteHub、BP_SharedCarryTask、ABP_CarrierRobot 编译通过。
- 目视检查：召唤师列表能水平滚动，天赋页保留纵向滚动区；工业区两名 Carrier 与共享箱体同时可见。验收结束后停止 PIE，关闭本轮 Slate 观察器，清理本轮临时验收图及遗留自动化存档；编辑器保留在主地图。
- 已验证充能整次多段、容量叠加/撤销不丢货、共享货物交付和中断守恒，以及超频完整阶段/换形计时、主线程全体/主力输出、轮选、死亡重选、新 AI 加成、身份限制和真实 UMG 状态更新。
- 临时抬箱为可替换的演示姿态，不是正式双人手部 IK 动画；碰撞和导航代理保持原尺寸。
- 狭窄矿坑路线曾触发队形阻挡回退，工业区宽通道交付已通过；需要更复杂地形时，应扩展队形导航，不应删除 Sweep 或强行传送。
- 本次没有改动 Blender 文件，也没有创建 Git 提交。

## 移动与右键转镜头的不同步修正

- OreBuddy 与人类当前 `RotationRate.Yaw` 均为 500°/秒，不能据此认定 OreBuddy 的配置转向更慢。动画身体骨骼和履带角度归一化检查没有发现额外的方向抢写。
- 确认存在一帧方向误差：UE 先执行 `ProcessPlayerInput`，各单位的 WASD 回调按更新前的 ControlRotation 加入世界空间移动输入，随后 `UpdateRotation` 才应用本帧鼠标旋转。角色跟随旧方向，镜头使用新方向；帧时变化会改变两者的夹角。
- `MineLearningPlayerController.cpp::UpdateRotation` 在引擎更新视角后，以本帧 yaw 差旋转待消费的移动输入。所有现有玩家单位的水平输入均为镜头相对 WASD；绕 Z 轴旋转保持红莲的升空输入不变。AI 不使用该玩家控制器，角色自身转身速度、动画、攻击自动转向与镜头碰撞规则均保持原样。
- 这里的输入契约是**玩家水平移动相对镜头**。以后若引入独立的世界方向移动命令，应明确分离其输入通道，不能让该命令经过这次镜头方向换算。此修正不处理 Root Motion 或 AI 导航。
- `Tests/UnitCameraOrbitAutomation.cpp` 使用主地图实际 OreBuddy，关闭开局菜单后，通过 W、右键和 MouseX 进入真实 Enhanced Input 链；在 PIE 临时空旷地面上交替 30/120 FPS，检查当前加速度方向、身体朝向、SpringArm、CameraComponent 与 CameraManager 的位置/角度。测试恢复原帧率限制，结束 PIE，不驱动桌面鼠标。
- 修正前，在镜头实际 450°/秒、身体 500°/秒的采样中，身体与镜头最大误差约 5.61°，身体瞬时转速会追赶到 500°/秒；修正后误差为 0°，身体同步为 450°/秒，变帧率专项测试通过。
- 把身体人为降至 60°/秒，或让镜头远快于身体时，身体仍会因转速限制而落后，这是正常转身过程；采样中的镜头不会因此被拖动。不要将其与上面的帧间方向误差混为一谈。本次未将背景抖动认定为已复现，也没有据此修改全局渲染参数。
- 最终完整编译通过；`OreBuddyCameraOrbit`、`GunnerCombatContract`、`PIEIntegration`、`UIFlow` 共 4 项测试通过。编辑器已重启载入新代码，验收结束停止 PIE 并回到主地图。
