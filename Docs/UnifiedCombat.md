# 统一伤害与属性检查

当前状态：统一伤害、Combat DataAsset、UMG 事件图已实现并保存。最新代码完整编译并链接 DLL 成功（`Saved/Logs/CombatFinalBuild.log`）；编辑器重启后四组自动化测试全部通过。PIE 已验证关卡采掘、属性面板开关与返回当前单位、换形态刷新、GM 扣血、红莲 Q 处决及降临未达线时存活。

## 数据与结算

- `UCombatConfig` 保存单位基础三维、最大生命，以及每个技能的固定值和三维系数。代码字段为 `Strength / Agility / Intelligence`，玩家显示为“力量 Strength / 敏捷 Agility / 能量 Energy”。不增加体力属性。
- `UCombatComponent` 读取配置与当前属性加成，生成实际伤害和 UI 展示数据。当前版本没有新增永久成长、存档或跨幻化成长机制。
- 普通伤害为 `(BaseDamage + Strength × StrengthScale + Agility × AgilityScale + Intelligence × IntelligenceScale) × Multiplier`。
- 所有正式扣血通过 `UCombatDamageSubsystem`，统一检查权威端、存活状态与敌我关系。友军不能互伤，矿物属于可受伤资源。
- `UHealthComponent` 持有唯一生命值，正式扣血函数仅供伤害子系统调用。回血有独立接口。
- 矿物最大生命仍取现有 OreDefinition；破损阶段、掉落与耗尽事件使用新的生命结算结果，不另存生命值。

## 技能接入

| 单位 | 结算点 / SkillId | 配置位置 |
|---|---|---|
| OreBuddy | 钻头实际命中 / Primary | DA_OreBuddyCombat |
| Gunner | 每发实际射击 / Primary | DA_GunnerCombat |
| 红莲 | 动画 HIT_A1、HIT_A2、HIT_A3 / Primary | DA_GurenCombat |
| 红莲 Q | GrabContact / QGrab；StartDissolve / QRadiation | DA_GurenCombat |
| 红莲降临 | 最后爆发 / Arrival | DA_GurenCombat |

目标配置目录为 `/Game/MineLearning/Combat`。技能栏的图标继续由已有 WBP 配置；数值说明与属性面板读取 Combat 展示数据。各技能保留原有动画和输入方式，不根据延时猜平 A 命中。

Gunner 普通爆头基础倍率为 4，黄金爆头为 6；Intelligence 分别增加 `0.005` 与 `0.01` 倍率，上限分别为 8、12。这些专属机制参数在 Gunner 的 Combat 分类中配置，黄金爆头通过普通伤害结算，不再直接秒杀。

红莲斩杀参数在 Q 组件的 `Combat | Execution` 配置，降临复用这份配置：`ExecuteHealthPercent = 0.5`、`ExecuteHealthFlat = 1000`。最终线为二者对应生命数值的最大值，即满足任一条件即可。

- Q：选取与真实接触时检查临界线；真正抓取成功后锁定处决，之后回血不能取消。前两段伤害保留至少 1 HP，最后通过统一入口处决。
- 降临：进入最后降临阶段时记录本次命中前的判定，随后才开始最终溶解；本次普通伤害不能反过来触发处决。未达到线的目标保留至少 1 HP，保留穿刺、蓄能等效果，最终改为爆炸并恢复材质。
- 抓取组件仅管理预留、附着和恢复，不能自行销毁单位代替死亡结算。

## 属性 UI 与 GM

交互：左键选中有生命组件的单位，点击右上“单位属性”或按 `I` 查看；“查看当前操控单位”返回当前形态。面板跟随 PlayerController，换形态重绑数据源；属性、生命、弹药及 Q 目标变化通过事件更新，没有 UI Tick。面板资产是 `/Game/MineLearning/UI/Combat/WBP_CombatDetails`，技能栏 tooltip 读取同一份技能计算数据。仅两个技能图标参与鼠标命中，其余 HUD 保持穿透。

非 Shipping 构建可在游戏控制台使用：

```text
CombatSpawnDummy 5000
CombatSetHealth 2500
CombatDamage 100
CombatHeal 5000
```

生成靶子后自动选中它；后三条操作当前选中单位。降低生命和伤害命令通过统一伤害入口，不能使用负伤害回血。降临保护阶段禁止外部扣血。

测试场景：5000 最大生命的目标在 2500 时可被 Q 选中，2501 时不可；1800 最大生命的目标在 1000 时由固定线准入。抓取接触后使用 `CombatHeal 5000`，应仍然处决。降临命中前为 2501 的目标应幸存，即使本次伤害将它打到 2500 以下。

## 验证

以下四个测试在最终 DLL 重启后通过 MCP 运行，4 passed / 0 failed：

- `MineLearning.Combat.DamageAndExecution`：正式 OreBuddy 配置加载、公式与面板同源、敌我、非法输入、双阈值、Q 抓后回血仍处决。
- `MineLearning.GurenUltimate.Lifecycle`：降临生命周期、双阈值、未达线目标跨线后仍存活。
- `MineLearning.GurenQ.GrabbableLifecycle`：抓取恢复与完成不能绕过统一伤害直接销毁目标。
- `MineLearning.PlayerForm.GunnerCombatContract`：枪手输入与射击契约；修正原测试误认为有七个输入绑定的旧断言，实际为六个，未改变原输入绑定。

后两类角色夹具中的无网格 socket / 换弹动画回退警告来自测试用原生角色，不是测试失败。

PIE 已观察到红莲 3000 生命、45/50/60 三维，以及普攻 97.25、Q 抓取 68、辐射 96、降临 376.5 的计算展示；Gunner 面板为普通伤害 91、爆头倍率 4.3、黄金倍率 6.6。GM 对 5000 生命靶子扣除 2500 后面板实时变为 2500/5000。运行时技能图标的 tooltip 文本也读取到了同一公式；悬停弹窗本身未取得独立视觉验收证据。

实际降临测试中，5000 最大生命、当前 2501 的靶子完整走过技能阶段后剩余 2124.5，仍然存活；随后实际 Q 动画完成处决。关卡 OreBuddy 产生连续 MiningHitConfirmed，矿物生命下降、破碎掉落与搬运继续运行。测试临时放置的红莲已移除，GameMode 默认玩家类保持原有人类；最终 PIE 已停止，未保存运行时测试靶子。

现有项目技能输入仍主要用于本地单人玩法。本轮保证扣血发生在 Authority，不宣称已补齐远程客户端技能 RPC、网络预测或完整多人表现。
