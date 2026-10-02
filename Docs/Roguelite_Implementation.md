# 肉鸽玩法实现说明（DT / DataAsset / UMG）
更新日期：2026-10-02。运行地图：L_WorldLayout_P01。

## 玩家流程
1. PIE 开始显示召唤师选择。身份免费；锁定身份需先在天赋树解锁。点击“开始本局”后，身份和天赋资格冻结。
2. 通关获得 1 天赋点，同一 RunId 不重复领奖。永久点数、研究和默认身份保存在 Saved/SaveGames/TalentProfile_v2.sav。
3. 按 T 或点击 HUD“天赋”打开节点界面。每个现有节点花费 1 点；超级弹夹需要超级子弹前置。研究只授予购买/抽取资格，局内不能研究。
4. 出生点附近有带“升级商店”名字牌的 Cube；靠近按 E，或点击出现的 HUD 商店按钮。
5. 消耗仓库金币购买抽取，依次为 **4、8、12、16…**，付款后立即显示最多三张不同且合资格的卡牌，点击一张生效。下一次价格 = 基础价格 ×（本局成功购买次数 + 1）。
6. 资源不足、没有合格内容、远离商店都不扣费。待选卡牌保留；关掉菜单再交互恢复选择，不重复付款。选择或清除待选不会降低价格，新局重新从 4 开始。
7. 黄金 AK 和复制人幻影与普通升级共用一个付费三选一池，满足身份/天赋/形态条件才入池，**没有单独购买召唤师能力的按钮**。
8. 按 B 或点击“图鉴”查看全部已配置升级卡，分“本局可抽”和“本局不可抽”。卡牌标明类别和适用单位（如 Gunner）；不能出现的卡显示主要阻挡原因。鼠标悬停查看完整说明。
9. Tab 保留生产终端：制造、出售、购买机器人/幻化、补弹匣等。Gunner 局内花费 4 铁矿购买；红莲先研究，再花费 10 铁矿购买。形态购买不赠送已装填弹药。
10. F6 保留准备页/GM 快捷入口，正式玩法不再依赖隐藏的调试列表。GM 页“清空存档 → 确认”会清除进度并重开当前地图。

| 升级 | 条件 | 默认效果 |
| --- | --- | --- |
| 刃舞 | 基础池，最高 3 层 | 每层攻速 +50%，改变攻击节奏与动画 |
| 疾风步 | 基础池，最高 3 层 | 每层移速 +50%，地面行走和冲刺 |
| 灵光一闪 | 基础池，1 层 | 施法速度 +100% |
| 力量强化 | 基础池，最高 3 层 | 每层力量 +50 |
| 搜集狂热者 | 天赋资格 | OreBuddy / Carrier 成功拾取后 50% 概率移速、施法速度各 +50%，5 秒；重复触发刷新 |
| 三连射 | 天赋资格、本局已购 Gunner | 每次射击三发，逐发扣弹与计数 |
| 超级子弹 | 天赋资格、本局已购 Gunner | 第 10、20、30…发真实射出的子弹伤害和命中特效尺寸 ×2，HUD 预告 |
| 超级弹夹 | 天赋资格、本局已购 Gunner | 换弹完成后 50% 概率基础 20 发变 40 发，不递归变成 80 |
| 黄金 AK | 氪金玩家、本局已购 Gunner | 黄金材质，普通射击伤害 +50%，黄金爆头率 +5 个百分点 |
| 复制人幻影 | 复制人 | 幻化支持 AI 的单位时生成最多一个幻影；三速各 +50%，蓝色半透明，30 秒后退场 |

红莲暂不支持 AI，人类也不生成幻影。取得能力时若已处于支持 AI 的形态，会立即生成。替换、回到人类、死亡/销毁、局结束与离开地图会清理幻影；退场立即停止逻辑，0.65 秒淡出。

## 代码职责与分层
以下文件以 Source/MineLearning/ 为根，名称对应 .h/.cpp。

| 文件 | 职责 |
| --- | --- |
| Roguelite/MetaProgressComponent | 永久点数、研究、身份、存档、胜利去重；不认识武器、Buff、UI |
| Roguelite/RunBuildComponent | 锁定当局资格/身份、已购升级层数、资格阻挡原因；不认识抽取价格、场景商店 |
| Roguelite/UpgradeDraftComponent | 合格池、权重、不重复候选、递增价格、扣费与一次选择；注入形态购买查询，不依赖 Demo |
| Roguelite/RunContentCatalog | 四张 DT、内容与配置校验；图标/短说明随内容配置 |
| Roguelite/MineRunCoordinatorComponent | 上层装配，连接经济、进度、构筑、抽取、拾取事实、单位效果与幻影；校验商店距离后发出购买意图 |
| Roguelite/RunContentQueries.cpp | Coordinator 的只读展示查询：卡牌、天赋节点/连线、身份、余额/价格/按钮可用性；复用同一资格规则 |
| Roguelite/RunContentView.h | 只读展示数据。UI 不重算资格、概率、价格 |
| Roguelite/RogueliteShop | 通用场景交互范围与进入/离开事实；不扣钱、不抽卡、不创建菜单，不依赖 Coordinator 或 Widget |
| Combat/UnitEffectDefinition、UnitEffectComponent | 可配置效果定义与实例、来源、事件、时长、刷新和撤销；不认识天赋/召唤师/拾取组件 |
| Combat/CombatModifiers、CombatComponent | 固定值与百分比贡献、三速/伤害/射程/黄金概率，兼容原 CombatConfig |
| Combat/WeaponActionComponent | 实际弹药、容量、弹夹、按来源计数、超级弹预告，射击/换弹准备与提交 |
| Combat/AmmoInventoryComponent | 控制器拥有备用弹匣，预留/完成扣除/取消释放 |
| Combat/UnitMovementComponent | 移速应用到 CharacterMovement 和原冲刺倍率 |
| AI/GunnerCharacter | 左键持续射击、首发转身、逐发结算、攻击/换弹动画速度、命中特效上下文 |
| AI/AutonomousUnit.h | 自主 AI 能力接口；Gunner/Carrier/OreBuddy 实现，红莲不实现 |
| AI/PhantomCompanionComponent、UnitRetirementComponent | 幻影生成/替换/寿命/清理、通用退场；不认识身份或天赋 |
| Presentation/PhantomPresentationComponent | 全息材质/淡出，静态附件非 Nanite 支持半透明 |
| Presentation/WeaponAppearanceComponent | 按外观 ID 配置材质，黄金不覆盖幻影全息材质 |
| Presentation/SummonerNameplateComponent | 玩家人类头顶 WidgetComponent 生命周期；具体文字在 WBP_SummonerNameplate |
| MineLearningPlayerController | 界面宿主：可替换菜单类、页面与 E/T/B/F6 输入、打开关闭、范围事件订阅、GM；不找控件、不设置文字/图标 |
| Demo/DemoRunComponent、DemoGuidance.cpp | 原生产/通关/资源消费、注入开局及形态权限，简化状态和下一步提示 |
| PlayerTransformZone、Manifestation/TransformationTypes.h | 原换形事务成功事实、独立形态枚举 |

Shop 是下层场景事实；钱包和抽取是业务层；Coordinator 装配业务；PlayerController 管理界面生命周期；UMG 负责呈现。去掉 UI 不影响交易规则，去掉商店 Widget 不影响范围判断。没有引入 GAS、MVVM 或额外万能事件总线。

## 调用链
购买：Cube 范围事实 → PlayerController 显示入口 → E 打开 Shop 页 → UMG BuyDraft → Coordinator 校验本局与附近商店 → Draft 构造混合候选 → Wallet 原子扣费 → 成功购买次数 +1 → OfferChanged → 宿主切换 Draft 页 → UMG 创建卡牌 → 点击广播 ID/候选序号 → Coordinator.ChooseUpgrade → Draft 二次校验 → RunBuild.Acquire → 已消费候选广播。

图鉴：Profile/Build/Draft/Run/Feedback 事件 → WBP_RogueliteHub.RefreshMenu → Coordinator.GetUpgradeCards → Draft.GetEligibilityBlock → Build 资格/身份/等级校验 + 注入的形态查询 → UMG 按可抽/不可抽分组，显示 DT 图标和短说明。展示与抽取使用相同校验，不维护第二份池子。

搜集狂热者：研究资格 → 开局冻结 → 付费选择 → 构筑变化 → Coordinator.AssembleUnit 按范围/单位类型 GrantDefinition → Carry.OnPickupCompleted → 上层转发 Effects.HandleEvent("PickupCompleted") → 效果概率/时长 → Combat.SetModifier → 单位动作消费倍率 → 到期移除临时贡献。

超级子弹：装备 ShotSequence → Weapon.PrepareShot → 来源计数修正本发 → 真正消耗弹药并发射 → CommitShot 计数 → 武器事件更新 HUD。空枪不计数，三连射逐发提交，换形往返迁移弹药与来源计数。

换弹：预留备用弹匣 → 中断则取消 → 完成才扣一枚 → 效果修正本次容量。成功幻化 Gunner 后，Coordinator.Transformed 调用 Gunner.TryLoadEmptyMagazine：仅空枪且有备用弹匣时立即完成首次装填，正常扣一枚，并经过相同的容量效果结算；无备用时保持 0，已有弹药不补满。Gunner 只认识弹药账户和武器动作，不认识召唤师或 UI。

弹药表现：WeaponAction.OnWeaponStateChanged → Gunner.NotifyAmmoChanged → OnAmmoChanged。BP_Gunner 的界面装配函数连接 WBP_V2_GunnerAmmo.ObserveGunner；Widget 先解绑旧来源，再订阅并立即读取实际弹药，销毁解绑。修复旧函数入口断线和错误引用旧版 Widget 的问题，默认占位由 ×10 改为 ×0。

幻影：BuildChanged 激活能力 → 换形事实/即时支持形态 → PhantomCompanion.SpawnFor → 自主 AI 接口 → CompanionChanged 装配三速与表现 → 通用寿命退场。

## UE 配置
Content Browser 打开 /Game/MineLearning/GameplayRuntime/。

| 资产 | 编辑内容 |
| --- | --- |
| Data/DA_RunContentCatalog | 四张表、DraftCost 基础价格、默认形态资格、胜利点数、存档槽、幻影配置；DefaultShopClass 与 DefaultShopTransform |
| Data/DT_Talents | FTalentNodeRow：ID、名称、说明、Cost、Prerequisites、Grants、图标、Position（节点连线取同一坐标） |
| Data/DT_Summoners | FSummonerRow：身份 ID、名称、简述、Icon；不直接赠送能力 |
| Data/DT_Upgrades | FUpgradeRow：资格、身份、形态、Weight、MaxRank、Audience、UnitClasses、Effects、幻影能力开关，以及 Category/ShortDescription/Icon/UnitLabel；UnitLabel 仅展示，UnitClasses 仍负责实际作用范围 |
| Data/DT_FormPurchases | 形态与资源价格 |
| Effects/DA_AttackHaste、DA_MoveHaste、DA_CastHaste、DA_Strength、DA_Collector、DA_GoldenAK、DA_PhantomHaste | UnitEffectDefinition：Rule.Id、触发事件、概率、时长、Modifiers；DA_MoveHaste 的 Id=MoveHaste50，MoveSpeed=0.5 |
| Effects/DA_TripleShot | RoundsPerAttack，当前 1–3 |
| Effects/DA_SuperRound | EveryN、DamageMultiplier、ImpactScale |
| Effects/DA_SuperMagazine | Chance、CapacityMultiplier |
| World/BP_RogueliteShop | 继承原生范围 Actor：Cube 外观、被动屏幕名字牌 |
| Materials/M_Phantom | HologramColor、Opacity、Visibility 淡出参数 |
| UI/Art/T_RogueliteIconAtlas | 九格实际图标纹理 |
| UI/Art/M_RogueliteIcon 与 MI_Icon_* | UI 材质，Scale/Offset 选择图集区域，可直接更换各行 Icon |

商店默认位于 (280,1550,310)。运行装配发现关卡没有商店时，按 Catalog 配置生成一个 Cube；关卡已摆放商店时优先使用。可在 UE 改位置/类，或直接摆 BP_RogueliteShop。商店本身只发布范围事实，不持有上层业务。

UI 文件：

| Widget Blueprint | 用途 |
| --- | --- |
| UI/WBP_RogueliteHub | 准备、商店、三选一、天赋、图鉴、GM 六页；所有文字/显隐/状态响应在蓝图 |
| UI/WBP_UpgradeCard | 身份/升级/图鉴复用卡牌，图标、分类、标题、两行效果、状态、Tooltip；只广播选择意图 |
| UI/WBP_TalentNode、WBP_TalentLink | 数据坐标节点和前置连线，点击广播研究意图 |
| UI/WBP_RogueliteShopSign | 被动 E 交互提示 |
| UI/WBP_SummonerNameplate | 人类头顶当前召唤师名字，订阅 Profile/Build |
| /Game/MineLearning/UI/V2/Widgets/WBP_V2_DemoRun | 精简状态、目标和下一步；生产终端保留 |

旧 WBP_RunMenu/WBP_ContentEntry 和专属 BuySummonerDraft 通路已移除。UI 无 Tick、轮询 Timer、循环 Delay、每帧属性绑定。页面先绑定事件再初始刷新，销毁成对解绑；卡牌销毁清理自身选择事件。当前单位变化先解绑旧 Pawn，装配完成再绑定新 Pawn；效果显示剩余时间由效果组件事件提供。

图标源文件位于 ArtSource/UI/Roguelite/：原图集仍服务力量、弹药、黄金 AK、幻影；Buff_Attack/Move/Cast/Collector.png 分别服务连斩、疾跑靴、施法手势、抓取矿石。对应 T_Buff_* 和 MI_Icon_* 使用同一 UI 材质；独立图标 Scale=(1,1)、Offset=(0,0)。生成记录见 ArtBriefs/RogueliteUI.md。材质和蓝图均通过 Unreal MCP 创建并保存。

## 配置扩展示例与边界
- 力量 +50：复制 DA_Strength，独立 Rule.Id，Attributes.Strength=50，新升级行引用。
- 普攻 +50 点 / 总普通攻击伤害 +50%：PrimaryDamageFlat=50 / PrimaryDamage=0.5。
- 每 5 发双伤害：复制 DA_SuperRound，EveryN=5、DamageMultiplier=2，新升级引用；Gunner 不新增 ID 判断。
- 换弹必定双容量：复制 DA_SuperMagazine，Chance=1、CapacityMultiplier=2。
- 新身份：身份行 + 天赋 Summoner Grant + 升级 RequiredSummoners；仍付费抽取。
- 新卡牌：设置 Category/ShortDescription/Icon。三选一和图鉴自动使用，无需增加控件分支。
- 调整价格：Catalog.DraftCost 的 Coin=6，则依次为 6、12、18…。使用同一资产配置实际交易与按钮当前报价。

百分比使用小数。同类来源相加：两个 +50% 为 ×2。属性为（基础 + 原核心固定加成 + Buff 固定加成）×（1 + 属性百分比合计）。普通攻击为（原公式按最终属性计算 + PrimaryDamageFlat 合计）×（1 + PrimaryDamage 合计）。攻速/移速倍率最低 0.1，不再以倍率 10 封顶；最终地面移动不超过 1000 cm/s，普通攻击最多 5 次/秒。施法/射程倍率仍限制 0.1–10。同来源重复装备不叠加；攻击叠层效果以同一个来源更新贡献和层数。

新事件需从真实业务发布，由上层转发。完整技能替换/觉醒、换弹不消耗弹匣、仅本弹夹射速等新行为仍需明确运行实例或技能槽契约，配置不是任意逻辑解释器。射程 +100%、攻速 -50% 可配置 AttackRange=1、AttackSpeed=-0.5；普攻距离已接入 Gunner、OreBuddy、红莲三连击和 Carrier 装卸；Q/R 等独立技能继续采用各自技能距离。Human 没有攻击，不虚构一个攻击入口。

| 动作 | 读取倍率 |
| --- | --- |
| 地面行走/冲刺 | 移速（红莲飞行除外） |
| Gunner 左键 / AI 射击 | 攻速：持续间隔、三连发时序、Montage |
| Gunner R | 施法：Montage 与无动画计时 |
| 红莲三连击 / Q、R | 分别攻速 / 施法，含技能阶段和溶解表现 |
| OreBuddy 挖矿 / 拾取交付 | 分别攻速 / 施法 |
| Carrier 装卸 | 1 + 攻速加成 + 施法加成，相加不相乘 |

## GM 与验证
F6 或导航 GM：补天赋点、补资源、解锁测试形态、添加攻速/施法/超级弹/弹夹/三连射、黄金/幻影测试升级、换弹概率控制、清理 GM Buff、清空存档。GM 是正常付费规则的明确测试例外。

控制台：BuffList、BuffAdd AttackHaste50、BuffAdd MoveHaste50、BuffAdd CastHaste100、BuffAdd SuperRound、BuffAdd SuperMagazine、BuffAdd TripleShot、BuffRemove SuperRound、BuffClear。BuffAdd 第二参数 -1 沿用时长，0 常驻，正数限时；仅作用于当前单位。普通效果可直接测试；Conqueror/Roamer 保留真实命中/移动条件，正数参数只覆盖叠层持续时间，0 不会把它改成常驻属性。BuffClear 只移除 GM 来源。

Gunner 射速：BP_Gunner 的旧 FireInterval=1.5 秒已改为 0.7 秒，与 C++ 默认一致。单发实际间隔 = max(0.2, FireInterval / AttackSpeedScale)；Montage 播放速度 = AttackSpeedScale × max(1, MontageLength / FireInterval)，避免动画长度反过来拖慢射击。三连射逐发间隔 = max(0.2, FireInterval / BurstFireRateMultiplier / AttackSpeedScale)，BurstFireRateMultiplier 默认为 2；动画按源帧 2/6/10 的命中节奏同步。冷却中松开再按左键也会继续等待下一发。增加攻速不会直接修改武器基础间隔。

Tests/RogueliteAutomation.cpp 覆盖配置、递增价格/失败不扣费/待选防重复/新局重置、资格与存档。RoguelitePIEAutomation.cpp 通过真实 UMG 商店及卡牌获取混合池幻影，验证名字、AI 能力、三速、持续左键 和退场。RogueliteUIAutomation.cpp 通过真实按钮和原生按键绑定检查准备→天赋研究→身份→开局→Cube E→购买→选择→图鉴→中控→清空；使用独立存档槽，截图只读取 PIE 窗口，不模拟系统输入。

此前商店/UI 实装验证：Catalog、PaidDraft、TalentPersistence、UIFlow、PIEIntegration、Demo.NavigationContract、Demo.BossEconomyContract 七项均通过。经济测试读取当前形态成本，先升级腾出仓库容量再补料，并使用独立存档，避免改变玩家进度。

2026-10-02 图标与 Gunner 修订：Win64 Development Editor 构建通过；修复弹药蓝图后 UIFlow 通过，修正幻影断言后 PIEIntegration 最终通过。实际 UMG 显示无备用弹匣 ×0、有备用弹匣变身后 ×20；第一发立即扣到 19，不进入首次换弹动画。移速 GM 加成改变 CharacterMovement.MaxWalkSpeed，移除后恢复。最终持续左键 的 4 秒观测为基础 6 发、攻速 +50% 后 9 发，平均间隔约 0.705 秒 → 0.474 秒，倍率约 1.49；Montage 倍率比为 1.5。幻影检查使用“共享升级之上额外 +50%”，避免随机先抽到施法/移速卡时误报。卡牌测试检查四张 Gunner 专属卡的 UnitBadge 文本；实际 PIE 图鉴检查新图标与标签无裁切。未观察到 Blueprint Accessed None。仍有既存 CrowdManager/RecastNavMesh 初始化警告。

当前范围为本地单人 PIE，未宣称多人复制、全平台打包或通用技能觉醒完成。未改无关美术资产或暂存区、未提交，保留此前 Gunner padding 删除。测试结束后退出 PIE，保留编辑器。



## 2026-10-02 攻击叠层与三连发模式

### 玩家规则

- 致命征服者：普通攻击命中 +1 层，上限 8；每层普通攻击伤害、攻速各 +5%，满层射程 +25%。有效命中刷新 5 秒；超时先掉 1 层，此后每 0.5 秒掉 1 层，直到清空。
- 漫游枪手：Gunner 移动中普通攻击命中：左右 +2 层，前后 +1 层，每层移速/攻速各 +5%，无层数上限；有效移动命中刷新 3 秒，超时每 0.5 秒掉 1 层。任何未命中立即清空。
- 漫游枪手持有者若由 AI 控制，每次攻击有 10% 概率失手，包括零层状态；玩家不受此随机概率影响。详细说明由 EquippedModifiers.AIMissChance 生成，改配置会同步文案。Gunner 原有随机打空概率已删除，爆头/黄金爆头仍随机。
- 全局普通攻击最多 5 次/秒，三连发按每一发计数；地面行走/冲刺上限 1000 cm/s（UE 中为 10 m/s）。这是本项目指定数值，不声称等同于英雄联盟当前默认上限。红莲飞行仍用原飞行参数。
- 三连发需原有天赋资格、局内付费抽取。获得后默认开启，鼠标右键切换；模式选择保存在武器运行状态。三连发点按左键一次三发，长按不重复；单发支持长按。逐发射速为单发的 2 倍，但遵守上限。缺少动画也使用相同计时，不瞬间补发。
- 已有攻击动作允许玩家空放：Gunner 开枪、OreBuddy 钻采、红莲普通三连击、Carrier 装卸空动作。空放/超距不产生伤害，算未命中。AI 正常只对有效范围目标行动，漫游枪手额外概率可导致失手。
- Gunner 超距仍扣子弹，无命中效果，头顶气泡显示“太远了，靠近再打！”。普通距离默认 Gunner 900、OreBuddy 135、红莲 400、Carrier 260 cm，测量到目标可碰撞表面（Carrier 交互点沿用装卸空间语义）。

### 代码和依赖

`CombatComponent` 提供最终属性、距离判定、动作间隔上限和 `OnPrimaryAttackResolved(bHit, bMoving)` 事实；不知道具体卡牌或肉鸽进度。每种单位的攻击执行者在真实结算后报告一次动作，红莲一次挥击命中多个目标也只报告一次。伤害在每次子弹/挥击/钻击结算时读取属性，该次命中得到的新层数从后续命中开始影响伤害。

`Combat/AttackStackEffect.h/.cpp` 提供可复用 `UAttackStackEffectDefinition` / Instance：订阅攻击事实，维护一个层数，按每层/满层配置产生贡献。`UnitEffectComponent` 的既有计时器负责持续时间和掉层调度，无新增 Tick；HUD 使用 StackCount 和 OnEffectsChanged。撤销、死亡、EndPlay 释放委托及全部贡献。

`Combat/CombatConfig.h` 配置基础 AttackRange、MaxAttacksPerSecond、MaxMoveSpeed。`UnitMovementComponent.cpp` 负责地面最终限速；Gunner、MiningTool、Guren、Hauler 分别在动作时序/动画上应用攻速与最小间隔。`CombatDamageSubsystem` 再拒绝超距普攻。`CombatModifiers` 只聚合属性和通用 AI 失手概率，不检查 Roamer 等业务 ID。

`Combat/WeaponActionComponent.h/.cpp` 拥有已解锁攻击模式及所选模式，`FWeaponRuntimeState.bBurstEnabled` 随运行存档传递；模式改变广播 OnWeaponStateChanged。`AI/GunnerCharacter` 绑定右键，阻止三连发长按重复，并使用 BurstFireRateMultiplier 配置逐发间隔。UI 订阅既有弹药/武器事件；Gunner 不操作 Widget。

`Roguelite/RunContentQueries.cpp` 将效果派生描述合并进卡牌详情。`WBP_UpgradeCard` 将分类/单位标签覆盖在图标上，卡牌减少 30 px 高度；攻速/施法/移速名称改为刃舞、灵光一闪、疾风步。`WBP_V2_CompanionBark` 订阅 OutOfRange，`WBP_V2_RobotSkillBar` 显示射击模式。

### 新配置

- `GameplayRuntime/Effects/DA_Conqueror`、`DA_Roamer`：AttackStackEffectDefinition；Rule.Modifiers 为每层，FullStackModifiers 为满层额外加成，EquippedModifiers 为装备时贡献。MaxStacks=0 表示无层数上限，DecayInterval 为掉层间隔。
- `GameplayRuntime/Data/DT_Upgrades` 增加 Conqueror/Roamer，基础付费抽取池，MaxRank=1，AllOwnedUnits。不免费授予 Buff。
- `GameplayRuntime/UI/Art/T_Buff_Conqueror`、`T_Buff_Roamer` 与 `MI_Icon_*`：新图标；源 PNG 位于 ArtSource/UI/Roguelite。
- 测试命令：`BuffAdd Conqueror`、`BuffAdd Roamer`、`BuffAdd TripleShot`。前两项需实际命中才出现层数；AI 失手概率修改 DA_Roamer.EquippedModifiers.AIMissChance。

英雄联盟参考：[11.23 致命节奏旧版](https://www.leagueoflegends.com/en-au/news/game-updates/patch-11-23-notes/)、[14.19 致命节奏改版](https://www.leagueoflegends.com/en-sg/news/game-updates/patch-14-19-notes/)。[9.23 征服者](https://www.leagueoflegends.com/en-us/news/game-updates/patch-9-23-notes/) 说明了命中叠加自适应属性、满层伤害治疗的原型。本项目的 8 层、伤害/攻速各 5%、满层射程和掉层间隔采用此次明确约定，不冒充当前官方符文原样实现；本次没有吸血需求。

### 本轮验证结果

最终 Win64 Development Editor 构建通过。Catalog、WeaponEffects、AttackStacks、GunnerCombatContract、PIEIntegration、UIFlow 六项相关测试最终均通过。AttackStacks 使用实际 PIE 时间验证 5 秒后由 8 层减到 7 层、失去满层射程、再命中恢复、移动条件、未命中清零、AI 失手及上限；正式 BP_Gunner 验证超距/强制 AI 失手仍扣弹但不伤害，正常命中产生伤害。

实际左键持续射击 4 秒：基础 6 发、+50% 攻速 9 发；平均间隔 0.704 → 0.469 秒，实际倍率 1.502，动画倍率 1.5。三连发长按 2 秒严格只射三发，逐发间隔符合 max(0.2, 0.7 / 1.5 / 2)；右键真实绑定切回单发后恢复长按连射，UMG 模式文字由同一状态事件同步。镜头测试确认身体转向不会在 SpringArm 两次更新之间拖动或旋转视图。

实机检查卡牌角标覆盖图标、改名、模式提示及 Buff 列表避让。修复测试中将嵌套技能栏误判为独立顶层 Widget 的定位错误；测试未模拟 OS/Slate 输入。仍存在项目原有 CrowdManager/RecastNavMesh 初始化警告。本轮不包含多人/打包验收。


## 2026-10-03 枪械手感、方向叠层和状态表现

### 玩家行为

- Gunner 开火从 Q 改为鼠标左键，技能栏保留开火、换弹两格；R 换弹，已取得三连发升级后右键切换。单发支持长按，三连发每次点击三发。OreBuddy 挖矿随后统一改为左键；红莲 Q 保留独立技能。
- 玩家每发真实子弹会增加后坐力，射线按向上、向右爬升再向左延伸的近似“7”形偏移，同时叠加小幅随机散布。准心随射击张开，停火后恢复。这是借鉴 AK 的可控弹道形状，非 CF 参数复刻。
- AI 使用有效目标射击，不套玩家压枪散布；只有持有漫游枪手时保留配置的 10% 失手概率，爆头规则不变。
- 漫游枪手限定 Gunner（包括 Gunner 幻影/AI），付费解锁 Gunner 后才有抽取资格。移动且命中才叠层：左右 +2，前后 +1，斜向取实际水平速度的主要方向；玩家参考瞄准朝向，AI 参考身体朝向。撞墙静止或站桩不叠层。
- 每层移速、攻速 +5%，3 秒无有效移动命中后每 0.5 秒减 1 层；任何未命中立即清零。攻速/移速上限保持 5 发/秒、1000 cm/s。
- 狂热采用红橙色脉动轮廓光。默认每层提升 5% 视觉强度，20 层达到视觉满强度；玩法层数仍可增加。掉层变暗、清零关闭，不替换基础模型或黄金 AK 材质。
- 玩家攻击可伤害目标但超距时，显示自身当前普攻半径的红圈 2 秒。再次超距刷新计时；显示期间跟随位置与当前射程。近战空挥会检查当前瞄准射线是否指向射程外有效目标，纯空地不会无故弹圈。Human 无攻击入口。
- OreBuddy 摇臂使用绝对旋转，并在角色移动组件之后更新，避免身体转向影响两次相机更新之间的视图。
- 所有已配置效果拥有状态图标，HUD 使用 44×44 设计单位的图标与文本行；实际尺寸随 DPI 缩放。致命征服者更换为经典金斧与节奏箭头的 AI 融合图。

### 代码与调用方向

| 文件/资产 | 职责 |
| --- | --- |
| `Combat/WeaponRecoilComponent.h/.cpp` | 可配置角度序列、随机散布、停火恢复；广播 `OnRecoilChanged`，不访问 UI。恢复计时仅在存在后坐力时运行。 |
| `AI/GunnerCharacter.h/.cpp` | 左键输入；每发扣弹后调用后坐力组件，重新射线检测并结算。三连发每颗子弹采用当前视角。 |
| `Combat/CombatComponent.h/.cpp` | 判断实际横移；报告超距事实。通用战斗层不认识狂热材质和状态控件。 |
| `Combat/AttackStackEffect.h/.cpp` | 根据移动方向使用 `StacksPerHit` / `StacksPerStrafeHit`，仍共用一个层数、命中刷新和衰减状态。 |
| `Combat/UnitEffectDefinition.h/.cpp` | `RequiredTargetClass` 对所有授予入口做单位限制，包括 GM；不限单位时留空。 |
| `Combat/UnitEffectComponent.h/.cpp` | 活跃视图提供图标、格式化状态、视觉强度；状态事件驱动显示。 |
| `Presentation/CombatFeedbackComponent.h/.cpp` | 订阅超距事件与效果变化；管理红圈可见时长、红光动态材质；不写战斗数值。退出时解绑并恢复原覆盖材质。 |
| `Roguelite/MineRunCoordinatorComponent.cpp` | 装配表现组件；底层战斗组件不反向创建表现组件。 |
| `AI/MiningCompanionCharacter.cpp` | OreBuddy 相机旋转隔离和更新顺序。 |
| `WBP_V2_GunnerCrosshair` | 订阅/解绑后坐力事件，按状态设置准心大小，无 UI Tick/轮询。 |
| `WBP_BuffStatus` / `WBP_RogueliteHub` | 叶子状态行展示 Icon + StatusText；Hub 按效果事件重建行，保留原武器提示刷新。 |
| `WBP_V2_RobotSkillBar` | Gunner、OreBuddy 第一技能显示“左键”，红莲仍显示 Q。 |

调用链：左键 → Gunner → 武器扣弹/后坐力射线 → 伤害结算 → Combat 命中事实 → 叠层效果 → 属性重算及状态事件 → UMG / 狂热表现。超距分支仅广播事实，由表现组件显示红圈。后坐力的恢复状态同时供弹道和准心使用，不让准心动画决定命中。

### 可在编辑器修改的配置

- `BP_Gunner` 的 `WeaponRecoil` 组件：`SprayPattern` 每项为（横向角度、向上角度），单位度；`RandomSpreadDegrees=0.3`，`RecoveryDelay=0.85` 秒，`RecoveryPerSecond=12`。停火后恢复到零才完全复位序列；准心大小由同一 Heat 状态驱动。
- `DA_Roamer`：`RequiredTargetClass=GunnerCharacter`，`StacksPerHit=1`、`StacksPerStrafeHit=2`、`AuraPerStack=0.05`。原有 3 秒持续、0.5 秒逐层衰减、AI 10% 失手配置保留。
- `DT_Upgrades.Roamer`：`UnitClasses=[GunnerCharacter]`、`RequiredPurchasedForm=Gunner`、`UnitLabel=Gunner`；已同步短卡文案和详细描述。
- 所有当前 `DA_*` 效果的 `Rule.Icon` 指向对应 UI 材质，`DA_PhantomHaste` 使用幻影图标。新增效果只需配置自己的 Icon；HUD 不维护一份效果 ID 到图片的硬编码表。
- `/Game/MineLearning/GameplayRuntime/FX/M_AttackRange`：红色细圆环；`M_StackAura`：Intensity 参数控制脉动红光。组件公开材质与 `RangeDisplaySeconds`，默认 2 秒。
- `DA_GunnerCombat.Primary.Input` 和技能说明已改为左键。

### 验证

Win64 Development Editor 完整构建通过。Catalog、WeaponEffects、AttackStacks、GunnerCombatContract、CombatFeedback、PIEIntegration、UIFlow 均已通过。CombatFeedback 实测方向加层、图标引用、红光随层数增强/失手清除、真实超距扣弹显示圆圈和 2 秒退出、后坐力序列和恢复、OreBuddy 转身相机稳定。完整流程继续验证左键射速和三连发、右键切换、付费三选一及存档隔离。

仅自动化临时空地图测试出现既有 NavMesh 警告，裸 C++ Gunner 测试有无模型 Socket/动画回退提示；生产蓝图的流程未出现新的 Blueprint 运行错误。测试不发送模拟桌面输入。

最终 PIE 补充检查通过：实际 BuffList 的行数与活跃效果数一致，每行都有图标和文本；漫游枪手状态显示为 20 层，实际 UMG 准心通过后坐力事件放大，第一技能显示“左键”；红光和红圈在真实装配的玩家 Gunner 上可见，失手清光和圆圈超时均通过。射程圈是无碰撞水平平面，台阶/矿石会正常遮挡部分圆弧，不投射到垂直墙面。

## 2026-10-03 输入与鼠标收口

- 正常操作角色时使用 UE 原生 `FInputModeGameOnly`，隐藏鼠标并捕获视角输入；首个左键点击也会传给角色，不额外吞掉一枪。
- 打开交互界面时使用 `FInputModeGameAndUI`，显示鼠标并阻止角色移动、转向和技能输入。关闭最后一个界面恢复游戏输入；HUD、准心和 Buff 状态条本身不改变鼠标模式。开局身份选择界面属于交互界面，所以此时仍显示鼠标；开始游戏或关闭后隐藏。
- 唯一决策入口是 `MineLearningPlayerController::RefreshMenuInputState`，读取已有菜单状态，没有新建光标管理器或 Tick 检测。覆盖属性、幻化选择、仓库、终端、天赋、图鉴、商店、三选一、GM 等现有菜单。Esc 也能关闭属性面板。
- 删除 `MineLearningCharacter::NotifyControllerChanged` 和 `GunnerCharacter::ApplyLocalPlayerViewport` 中抢写鼠标/输入模式的逻辑；角色只管理自身镜头、移动和动作。
- `AI/MiningCompanionCharacter.cpp`：左键触发原 `TryUseMiningSkill`；移除旧 Q 输入绑定及无用 action 成员。R 拾取不变。`WBP_V2_RobotSkillBar.ConfigureForMining`、`DA_OreBuddyCombat` 同步标注左键。
- `AI/HaulerPlayerControl.cpp`：左键触发装卸；控制器 E 不再调用搬运动作。E 留给商店、终端及幻化等场景交互。`DA_CarrierCombat`、`DemoGuidance.cpp`、`DemoRunComponent.cpp` 和货物提示同步更新。
- 红莲左键已有三连击，Q 是抓取技能，故保留 Q；没有发现其他需要保留 Q 的玩家单位。

验证：Win64 Development Editor 完整构建成功；Catalog、WeaponEffects、AttackStacks、CombatFeedback、PIEIntegration、UIFlow、GunnerCombatContract 共 7 项全部通过。扩展的 UIFlow 调用实际 UMG 按钮与原生按键绑定，验证 OreBuddy 左键启动空挥挖矿、Carrier 左键空目标动作、旧 Q 绑定移除、菜单打开/关闭与叠加面板、菜单期间换形、回到人类的光标与输入状态。清档会重载关卡，故作为测试最后一步。没有模拟桌面输入；既存 NavMesh 初始化及裸测试 Gunner 缺少骨骼/动画的警告不属于本轮新增错误。
