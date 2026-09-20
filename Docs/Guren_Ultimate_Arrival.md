# 红莲 R 大招「降临」——多目标穿梭版

更新：2026-09-20；UE 5.8.1。最终镜头改为前景红莲与后景辐射目标共同构图。保留 1200 cm 起手和降临高度、1.5 倍飞爪速度、浅弧转移和短拖尾。

同日视觉修订：参考 `ref_19_cg_full_wing_eclipse.png` 的日蚀构图，将降临细双线替换为白粉亮核、洋红外晕和较弱的外圈余辉，中心保持透明。幻化卡片与 R 图标改从实际 Arrival 阶段取姿态，分别构图，不再使用孤立动画片段采样的站姿或把同一全身图直接缩成技能小图。

## 操作

挖矿主场景 `ThirdPersonMap` 也已接入红莲：进入机器人研发中心的幻化区域，按 **E** 打开卡片，按 **3 / 小键盘 3** 变成红莲。底部技能栏显示 **Q 辐射抓取**、**R 红莲降临**及各自图标，**TAB** 切换抓取候选目标；空格、WASD 和原有飞行控制保持不变。回到研发中心按 **E → 0** 恢复人类，1/2 仍选择 OreBuddy/Gunner。

玩家使用独立的 `BP_GurenPlayer`（继承 `BP_GurenCharacter`），已关闭自动玩家占有和自动 AI 占有，AI Controller 为空；研发中心生成后由当前玩家控制器接管。本轮仅接入变身和已有技能，不新增红莲 AI，也不调整技能筛选、伤害及结算边界。

打开 `/Game/MineLearning/Maps/L_Guren_Retarget_Test`，Play 后靠近测试对象按 **R**。地面、飞行状态均可释放；优先采用 Q 当前选中对象，否则查找范围内最近的可见目标，再按最近邻顺序串联其他对象。TAB 可在释放前切换 Q 候选目标。一次默认最多贯穿 **6 个**目标，重启 PIE 恢复测试对象。

使用场景中的真实 Gunner、OreBuddy、小白人、矿石、仓库和售卖机，全部通过 `UGrabbableComponent` 预约和结算。演出中禁止 Q、跳跃和重复 R；结束后恢复移动、视角和技能输入。正常结束在配置高度悬停，随后交回原有飞行控制，松开输入会按已有飞行规则下落。

## 流程与时序

| 时间 | 阶段 | 行为 |
| --- | --- | --- |
| 0–1.35 s | Ready | 预约整组目标；保存输入、移动与伤害状态。角色调整到脚底离地 1200 cm，朝向目标群；进入夜空氛围和全景 |
| 1.35–3.22 s | Launch | 隐藏原右前臂，直达首目标，随后沿浅弧线依次穿过其他目标；飞爪后只保留一小段红色尾迹 |
| 3.22–3.58 s | Impact | 飞爪从最后目标沿浅弧线返回右臂挂点；所有目标仍保留实体，持续红热 |
| 3.58–4.88 s | Descent | 收回飞爪、恢复原手臂；角色回身、展开双翼和光环，镜头推近。当前最终高度同为 1200 cm，不再降回低空 |
| 4.88–5.78 s | Arrival | 前景保留降临姿态与光环，后景展示受击目标红热变色；尚未提交目标完成结果 |
| 5.78–6.43 s | Burst | 保持特写，已贯穿目标同时触发辐射崩解与扩散环，各自提交一次完成结果 |
| 6.43–6.88 s | Recover | 光环淡出、光照恢复、混合回游戏视角，释放输入和预约 |

上表为默认 1.5 倍飞行速度，时间四舍五入。`StageDurations` 保存 1 倍的基础时长，实际 Launch、Impact 时长除以 `FlightSpeedMultiplier`；其他阶段不变，Montage 各段自动适配，不用另改动画资源。

地面与空中使用同一次向下探测计算高度，高于起手高度时下降、低于时上升。高度是**胶囊底部到地面**的距离，不是世界 Z。上方有障碍时胶囊扫描限制上升；没有有效地面时保持原高度。移动仍使用碰撞扫描。

首次攻击直接抵达目标；换目标和回收只使用单侧浅弧线，不再在一段内左右摆动。沿起终点连线的前进进度保持单调，侧向偏移不超过 `ArcWidth` 或该段距离的 15%，相邻目标转移交替选择弯曲方向。每个目标只经过一次，不生成围绕目标的环形轨迹。目标位置在预约时缓存，目标中途被外部销毁也不会留下死锁，其他目标继续正常演出和结算。

## 配置入口

挖矿形态在 `BP_GurenPlayer` 配置；独立测试场景使用 `BP_GurenRetargetTest`。选择以下组件，动画与材质均为正式 UE 资产，可在编辑器继续精修。两者均继承 `BP_GurenCharacter`，独立子蓝图上的参数覆盖需分别配置。

### 幻化卡片与技能栏

- `PlayerTransformZone.GurenPawnClass` 配置红莲玩家蓝图，使用现有统一换形流程保留脚底高度、接管控制器并清理旧角色。
- `WBP_TransformationSelection` 增加第四张红莲卡片；头像为 `UI/Transformation/Portraits/T_TransformPortrait_Guren`。
- `WBP_PlayerFormHUD.HandlePawnChanged` 在切换形态时清理旧界面数据源，红莲分支调用共享技能栏的 `ConfigureForGuren`，并创建 `WBP_GurenFlightHUD`。再次换形和宿主销毁时移除能量条，其状态委托随 Destruct 解绑。
- `WBP_RobotSkillBar.ConfigureForGuren` 使用 `UI/Guren/T_GurenGrabClaw` 和 `T_GurenArrival`，按原图颜色显示，不显示弹药数量。切回挖矿伙伴或枪手时恢复它们的图标、染色和状态订阅。上述表现全部由 UMG 蓝图负责，无 UI Tick。
- 新头像、R 图标直接渲染当前 UE 模型与降临姿态，正式源图保存在 `ArtSource/UI/Guren`，未改动模型、骨架或动画。

主场景 PIE 验证：E/3 变身；Q 完整经过 Dash、Grab、Radiation、Release 并回 Idle；R 对场内 4 个目标完整经过 Ready 至 Recover 并回 Idle，结束后移动和视角输入均恢复；人类 → OreBuddy → Gunner → 红莲 → 人类 → 红莲连续切换，能量条在红莲形态恰有一份，其余形态为零。

| 组件 | 参数 | 当前默认值 / 用途 |
| --- | --- | --- |
| Ultimate | Range / ChainRange | 3200 / 2600 cm；前者按水平距离筛选，后者限制相邻目标距离 |
| Ultimate | MaxTargets | 6，范围 1–12 |
| Ultimate | HitSocket | `RadiantHitSocket`；缺失时使用可见实体网格包围盒中心，排除 UI 和编辑器网格 |
| Ultimate | LaunchHeight / FinalHoverHeight | 1200 / 1200 cm，分别配置起手和最终展翼时脚底离地高度 |
| Ultimate | ArcWidth | 150 cm，换目标和回收的最大侧向弧度；短段自动限制为距离的 15%，首目标直达 |
| Ultimate | FlightSpeedMultiplier | 1.5；飞出和收回的速度倍率，1 恢复基础速度，不影响特写和结算节拍 |
| Ultimate | LaunchSocket | `radiant_forearm_r`，飞爪出发及回收位置 |
| Ultimate | StageDurations | Ready 至 Recover：1.35、2.8、0.55、1.3、0.9、0.65、0.45 秒 |
| UltimatePresentation | Montage / LaunchBone / ArmMesh | 分段动画、原前臂隐藏骨、独立飞爪网格；LaunchBone 与业务挂点保持一致 |
| UltimatePresentation | EstablishOffset | 沿攻击方向、侧向、上方的全景偏移系数；构图覆盖目标群、角色与曲线余量 |
| UltimatePresentation | ArrivalOffset / HeroCameraDistance | 降临机位方向 / 最小距离 1100 cm；机位随角色与目标高差适度抬高，避免只拍到天空 |
| UltimatePresentation | ArrivalTargetFramingWeight | 0.3；最终注视点从红莲胸口向目标群中心偏移的权重，越大越照顾后景 |
| UltimatePresentation | ArrivalMaxCameraDistance | 2200 cm；共同构图的最大拉远距离，防止目标分散时红莲缩成远景；有效上限不低于 HeroCameraDistance |
| UltimatePresentation | WideFieldOfView / FieldOfView / CameraBlendTime | 全景 76° / 特写 64° / 0.35 秒；镜头受墙面限制时仍保留较宽视野 |
| UltimatePresentation | HaloMesh / HaloMaterial / HaloRadius | 光环平面、材质、半径 330 cm |
| UltimatePresentation | TrailSegments / TrailWidth | 64 段 / 28 cm；复用平面池，按飞爪真实位置绘制渐细尾迹 |
| UltimatePresentation | TrailLength | 350 cm；按真实累计长度裁掉旧轨迹，速度提高或帧率降低也不会留下整条攻击路线 |
| UltimatePresentation | RadiationSystem | 复用 `NS_GurenPalmRadiation`，命中效果按目标半径缩放 |
| UltimatePresentation | bNightAtmosphere | 开启临时夜空、冷色环境光与角色补光 |
| UltimatePresentation | NightSkyMesh / NightSkyMaterial | 球形夜空与 `M_ArrivalNightSky` 星点材质 |
| UltimatePresentation | NightSunMultiplier / HeroLightIntensity | 原太阳强度乘 0.38 / 角色补光 180000；退出恢复原太阳参数 |
| UltimatePresentation | BeatSounds | 按七阶段排列；允许空项，Impact 音效在每次实际贯穿时播放 |

`M_ArrivalHalo` 的 `Intensity` 控制阶段强度：`Style=0` 保留目标结算扩散环，`Style=1` 保留飞爪尾迹，`Style=2` 为降临日蚀光环，由表现组件创建 Halo MID 时指定。三种形状共用现有材质资产，修改降临分支不会把飞爪尾迹或目标爆发改成粗光环。

日蚀分支的 `Eclipse Halo` 参数组：`CoreWidth=0.014` 控制白粉亮核半宽，`CoronaWidth=0.065` 控制外侧光晕范围（两者均为平面 UV 单位），`CoronaStrength=0.9` 控制洋红外晕与外圈余辉的亮度。透明中心保留机体、翼片及真实受击对象；宽度与亮度由材质调整，整体世界尺寸仍由 `HaloRadius` 调整。相较参考图，本版收敛外围发光，避免游戏内整屏泛白。

视觉回归覆盖独立红莲测试关卡与 `L_WorldLayout_P01`：实际 Arrival 姿态取图、研发中心卡片、变身后的 Q/R 技能栏、完整 R 演出及结束后的光环清理和控制恢复。UI 图片直接替换原 Texture 资产，既有 UMG 布局、绑定、Q 图标及选取/结算逻辑不变。

当前夜空是演出用临时天幕，并未修改测试关卡的永久天空；退出时销毁天幕、补光和演出摄像机。

最终镜头使用红莲、光环和预约目标的缓存范围计算构图，按 Filmback 视场留出边缘余量，并限制俯视角和拉远幅度。优先保持红莲姿势可读，让受击对象出现在后景；目标过度分散或被场景遮挡时不强求全部同时入镜。Descent 平滑转入该构图，Arrival 和 Burst 共用相同缓存范围，目标销毁不会引起重新取景。未修改角色朝向、目标位置、辐射材质或结算时间。

新目标需要可用的 `UGrabbableComponent` 和 Movable 根组件，不能被其他抓取预约、隐藏或附着在其他 Actor 上。其原材质需接入项目共享辐射溶解参数协议，才会产生表面红热和裁切；不会把未知材质强行替换成通用表面。

结算沿用 `Completion`：Destroy 销毁、矿石调用已有采矿完成事件、Restore 恢复本体并通知 `OnGrabCompleted`。本版“伤害结算”指这一已有完成契约，没有新增统一血量或 Boss 扣血规则。需要保留生命值的对象应在其完成事件中接入自身规则。

## 代码与生命周期

- `AGurenCharacter` 装配业务组件，绑定 R，并处理 Q、跳跃、飞行逻辑的互斥。
- `UGurenUltimateComponent` 是时间线唯一来源：负责选目标、预约、固定高度、曲线路径、贯穿事件、延迟结算和退出恢复。即使不挂表现组件也能完成业务流程。
- `UGurenUltimatePresentationComponent` 订阅 `OnStageChanged` 和 `OnTargetPierced`，负责 Montage、摄像机、飞爪、尾迹、Niagara、目标临时材质、声音和夜空。
- `AM_Guren_UltimateArrival` 使用七个与阶段同名的 Section。表现层按业务阶段跳转，并按配置时长自动调整播放速率。已删除旧的大招 Notify 驱动，不再依赖 Montage 正常结束释放输入；Montage 被中断会取消技能。
- `WBP_GurenFlightHUD` 继续事件绑定和解绑，演出中透明度 0.12、结束 1.0，无 UI Tick。
- `MineableOre` 的普通和致命采矿都尊重 `CanBeDamaged`，避免预约期间被其他采矿调用抢先结算。

每次命中只为该目标的可见网格创建独立 MID，保存每个材质槽的**原对象引用**与相对变换。贯穿后升温，Arrival 末段连续溶解并发射表面粒子，Burst 保持原时间提交完成。取消时所有目标统一释放预约并恢复材质，不按目标类型写特殊分支。

飞臂显示等待骨骼双缓冲发布“原前臂已隐藏”，避免同帧出现两只手。收爪后恢复原骨骼，独立飞爪和尾迹退出；粒子位置在飞爪更新后跟随掌心。没有修改骨骼网格的 Nanite 或导入设置。

所有退出路径汇合 `Exit` / `Cleanup`。取消恢复原始位置、移动模式和速度；正常结束在最终配置高度进入 Flying，交还角色控制。附近 AI、伤害许可、精确材质引用、太阳光、原 ViewTarget 和临时组件均成对恢复。外部既有移动/视角锁不会被 Reset 清除，使用成对增减。

### 无法控制的根因

旧代码用 `PlayerController::IsInputComponentInStack` 判断角色原先是否接收输入。这只检查控制器的附加栈，正常被 Possess 的 Pawn 输入由 `BuildInputStack` 另行加入，因此会误判为 false，调用 DisableInput 后未再 EnableInput。

现在由业务组件保存 `Pawn::InputEnabled()`，在正常结束和取消时恢复；摄像机表现层不再拥有角色输入开关。原本被其他系统禁用的 Pawn 仍保持禁用。自动化同时检查 Pawn 输入与控制器锁，避免只检查 `IsMoveInputIgnored` 再次漏掉此问题。

## 资源与保护范围

正式目录 `/Game/MineLearning/Characters/Guren/Ultimate/` 包含三个独立 AN、一个 AM、飞爪 SM、光环及夜空材质、五个占位声音。飞爪复用现有前臂与原材质；大招动作是现有姿势与机械翼关键帧构成的 UE 原型，允许后续替换专门精修动作和声音。

`ABP_GurenLocomotion` 的最终输出使用 ArrivalSlot，Skeleton 仅新增 Montage Slot。R19 Blender、既有骨架、Rest Pose、权重、UV、Q 动画和原飞行动作保持保护。本版以原生 CineCamera 和阶段事件组织演出，没有新增 Level Sequence。范围为本地单人 Demo，未实现多人网络演出同步。

一次性资产制作器、调试输入命令、编辑器模块依赖及截图脚本不保留在功能交付中。

## 验证与复现

2026-09-20 镜头复核：Development Editor 编译通过；真实 PIE 分别验证六目标和单目标释放。六目标画面同时保留红莲完整展翼/光环、红热矿石与设施；单目标画面保留红莲和发红的小白人。Burst 沿用该机位可见辐射爆发，两个用例均完整返回 Idle，游戏镜头和输入锁恢复。只修改表现层镜头，本轮未改动业务时间线、攻击轨迹与伤害契约。

UE 5.8.1 Development Editor 编译；自动化 `MineLearning.GurenUltimate.Lifecycle` 覆盖逐阶段取消、重复/乱序请求、Pawn 输入恢复、外部锁保留、原禁用输入保留、目标中途销毁、浅弧线单向推进且不绕圈、1.5 倍出发/回收时间及特写时长不变、独立最终高度配置、三个目标先贯穿后统一结算。

本轮 PIE：6 个目标完整经过七阶段并回到 Idle；角色世界 Z 1430.46 cm，扣除胶囊半高 230.46 cm 后脚底离地为 **1200 cm**。实际配置的飞出时长 **1.8667 s**、回收 **0.3667 s**；逐帧测得可见拖尾累计长度最大 **350 cm**（浮点误差小于 0.001 cm）。已核对短拖尾、浅弧转移、高空展翼画面，以及结束后游戏镜头和输入锁恢复。原生编译和更新后的生命周期自动化均通过。

上一版的真实 PIE 回归记录（保留历史数值，当前高度配置见上表）：

| 用例 | 结果 |
| --- | --- |
| 按 R 完整释放 | 6 个真实目标，Ready → Launch → Impact → Descent → Arrival → Burst → Recover → Idle |
| 结束后的真实按键 | W 移动约 536 cm、MouseX 转向约 27°，再次按 R 成功；不是仅检查布尔标记 |
| 高空起手 | 初始世界 Z 1732.61，起手定位 Z 1080.46，与地面起手相同；扣除地面和胶囊高度后脚底离地 850 cm |
| 全目标贯穿后、回收中取消 | 全部原材质引用恢复，太阳强度及颜色恢复，镜头和输入锁恢复，返回原空中位置 |
| 穿梭中外部销毁首目标 | 剩余演出正常完成，回到 Idle，Pawn 输入、镜头和太阳恢复 |
| 视觉检查 | 目标逐个升温、收爪、原臂恢复、展翼光环、夜空和结算特写 |

可在编辑器控制台执行 `Automation RunTests MineLearning.GurenUltimate`。Q 的四项生命周期、材质、表面覆盖和掌心特效测试已在首版大招交付时通过；涉及 Niagara 的检查需要真实 RHI，NullRHI 只用于此次纯业务生命周期测试。

手动复测：从地面和空中各按一次 R，观察最后一个目标在特写前仍存在；结算结束立即按 WASD、移动鼠标，再按 R。若目标已消耗完，重新 Play。

## 2026-09-20 后半段辐射崩解修订（上一版）

本轮依据《红莲_降临技能特效修复施工单》只调整表现层与 UE 特效资产。选敌、伤害/完成契约、七阶段业务时间线、角色模型、骨架、动画及 UI 源图不变。此前“Burst 直接透明 + 掌心小爆点 + 平面扩散圆圈”的实现由以下流程替代。

### 视觉流程

1. `OnTargetPierced` 为目标可见 Static/Skeletal Mesh 保存原材质引用，建立独立 MID，并绑定对应 GPU 表面采样器。命中后的红热保留原纹理、金属和表面明暗。
2. 回收、降临期间，目标逐步出现深红高热与局部能量纹；独立 TargetCharge 仅包含短黑红电弧和局部空气折射，不生成掌心核心或地面波圈。
3. Arrival 末段先产生短促白热，再连续推进共享材质的溶解半径，同时从真实模型表面出生粒子。白闪以开始溶解为基准，避免较宽 Bounds 的模型已消失才闪光。
4. 主体使用三维烧蚀薄片，伴随较快红白火星和较慢暗红灰烬。初始位置覆盖全身，速度含径向、切向与背离红莲的偏置；碎片冷却变暗，不使用大火球。
5. Burst 仍在原时间提交目标完成。表面粒子已经在结算前生成并以世界坐标独立飞散，不依赖被销毁目标继续存在。短电弧、折射先收束，灰烬稍后消退；退出统一回收组件并恢复仍有效目标的原材质。

背环使用红莲实际 Forward 与世界 Up 构造稳定竖直平面，位置由角色局部 `HaloOffset` 给出；随角色朝向和体积变化，保持小幅呼吸。它不是镜头 Billboard。斜看出现透视椭圆属于正常透视。

### 配置入口

在 `BP_GurenRetargetTest` / `BP_GurenPlayer` 的 **UltimatePresentation** 组件中配置：

| 参数 | 默认 | 含义 |
| --- | ---: | --- |
| Static Disintegrate System | NS_ArrivalDisintegrate_Static | 静态网格表面 GPU 采样 |
| Skeletal Disintegrate System | NS_ArrivalDisintegrate_Skeletal | 当前骨骼姿态表面 GPU 采样 |
| Target Charge System | NS_ArrivalTargetCharge | 黑红短弧、局部热折射 |
| Bounds Sample Mesh | Engine BasicShapes/Cube | 无可用表面时的隐藏包围体采样源 |
| Target Particle Budget | 190 | 每个主目标的总发射预算，按可见部件面积近似分配 |
| Secondary Target Intensity | 0.7 | 其他目标的发射量、发光及折射权重 |
| Disintegrate Lead Time | 0.32 s | 原 Burst 结算前的视觉溶解窗口 |
| Critical Flash Time | 0.09 s | 完整可见表面的短白热窗口 |
| Target Heat Intensity | 3 | 常规蓄能强度，与短白闪分开 |
| Halo Offset | (-85, 0, 45) cm | 角色局部背环中心偏移 |

当前主目标取已预约目标序列第 0 项；不为特效重选目标。六目标发射预算约 `190 × (1 + 5 × 0.7) = 855`，各部件/发射器实际数量受整帧采样取整影响。三层发射量比例为 20% / 55% / 25%；未增加目标独立动态灯或叠加多个屏幕震动。

### 正式资源与代码边界

新增资源位于 `/Game/MineLearning/Characters/Guren/Ultimate/`：

- `NS_ArrivalDisintegrate_Static`、`NS_ArrivalDisintegrate_Skeletal`：RadiantChunks、HighSpeedSparks、DarkAsh 三层 GPU 发射器。
- `SM_ArrivalFragment`、`M_ArrivalChunk`：有厚度的三维烧蚀碎片和冷却材质。
- `M_ArrivalSpark`、`M_ArrivalAsh`：速度方向火星及深色灰烬。
- `NS_ArrivalTargetCharge`、`M_ArrivalArc`：R 专用细短黑红电弧与共享局部折射层。

资源本身是可编辑的 UE Source of Truth；制作脚本和临时 C++ 工具不属于运行时依赖。

`UGurenUltimatePresentationComponent` 消费既有两个事件，控制视觉窗口、临时材质与采样器生命周期。`UGurenUltimateComponent` 不认识这些资源。共享 `MF_RadiantSurface` 增加默认值为 0 的 `RadiantStress` 与 `RadiantCritical`：R 的独立 MID 才写入，Q 与其他消费者保留原路径。结束恢复的是原材质对象，而非重建近似材质。

没有可用网格时，使用不可见 Bounds 采样源并向内随机采样，属于包围体近似，不能还原不存在的真实表面。未知材质仍需接入原有共享函数才能红热/裁切；不会强行替换其着色器。目标提前失效时停止该表面继续发射并安全清理，不会追加重复结算；此版本未新增“提前死亡”的业务或动画流程。

### 本轮实测

- UE 5.8.1 Development Editor 正式编译通过；一次性资产制作器和它使用的编辑器模块依赖已移除。
- 测试场景真实 PIE 覆盖小白人正面/侧面、大矿石近景及原有六目标降临特写。检查连续画面中的深红能量纹、短白热、身体裁切与飞散；网格尚存在时就开始出生粒子，不再依靠 Actor 消失后的地面爆点。
- 使用 GPU SimCache 确认 RadiantChunks、HighSpeedSparks、DarkAsh 均有实际粒子，Mesh Renderer 的 Scale 数据有效。三维碎片按中等尺寸收敛，灰烬缩小并软化边界，避免大黑色块遮住目标。
- 两次在 Arrival 阶段取消并重放，9 个受影响网格均恢复准确的原材质对象；镜头返回 Pawn，移动/视角输入锁解除。
- 在 PIE 隐去目标可见网格，确认仅创建一个不可见 Bounds 采样源；取消后该组件被清理。
- 关闭制作会话后重新启动编辑器，从磁盘验证共享热纹分支、8 个正式 VFX 资产和两个红莲 Blueprint 的引用均存在。
- 正式编译后以真实 RHI 再跑 `Automation RunTests MineLearning.Guren`，5 项全部通过：Q 的预约生命周期、材质保真、掌心特效生命周期、表面覆盖，以及 R 生命周期。表面覆盖测试新增断言：正式目标材质的 `RadiantStress` / `RadiantCritical` 默认均为 0，防止 R 的分支污染 Q。

复测入口仍是 `L_Guren_Retarget_Test`，Play 后按 R。优先观察降临末尾：目标保持可读的深红表面，白热后逐渐缺失并散出红白火星，随后残留暗色碎片与短电弧。近景和最终共同构图中的颗粒大小不同，背环在侧面会出现正常透视压缩。临时连续捕获帧、导出检查数据和制作脚本按项目美术文件规则清理，正式资源可直接在 Niagara/材质编辑器中继续调整。

## 当前版本：贯穿红色喷散 → Q 风格白色溶解

根据用户后续三张实机截图，最终表现改为 Q 风格。上一版的短白闪、终结红色喷散不再是当前配置。

1. **飞爪贯穿**：`OnTargetPierced` 为目标可见网格绑定独立的贯穿和溶解采样器。红色高速火星及少量碎片在命中后短促发射，目标继续保留实体，进入深红蓄能。
2. **回收与降临**：维持原有红热纹路、细电弧与局部折射。白色粒子此时尚未发射。
3. **结算前 0.6 秒**：切回 Q 使用的共享表面着色分支，持续提高剩余表面的白热亮度并推进裁切；同时复用 Q 的细密白色表面粒子。替代原来仅闪约 0.09 秒的全身白光。
4. **结算与退出**：沿用原业务时间结算，粒子自然消退；正常结束、取消和 EndPlay 统一回收两套采样器，恢复仍有效目标的原材质对象。

### 当前配置

两个红莲 Blueprint 的 `UltimatePresentation` 使用相同配置：

| 参数 | 当前值 | 用途 |
| --- | --- | --- |
| Static / Skeletal Pierce System | NS_ArrivalDisintegrate_Static / Skeletal | 上一版红色喷散资源，现用于贯穿命中 |
| Pierce Particle Budget | 64 | 主目标单次贯穿的总采样预算，各网格按面积近似分配 |
| Pierce Emission Time | 0.12 s | 命中短喷窗口 |
| Static Disintegrate System | NS_RadiantDissolve_Test | 直接复用 Q 的静态表面白色粒子 |
| Skeletal Disintegrate System | NS_RadiantDissolve_Skeletal | 直接复用 Q 的骨骼表面白色粒子 |
| Target Particle Budget | 1500 | 主目标终结窗口内的总采样预算；实际存活数受溶解前沿筛选影响 |
| Disintegrate Lead Time | 0.6 s | Burst 前的连续白色溶解窗口 |
| Dissolve Heat Intensity | 6 | 白热阶段发光强度，与之前的深红蓄能强度分开 |
| Secondary Target Intensity | 0.7 | 其他目标的采样量与发光权重 |

`CriticalFlashTime` 已移除。白热不再由独立脉冲驱动，直接跟随溶解进度。白色采样沿用 Q 的 45 cm/s 基础速度和 0.18–0.30 秒粒子寿命，保留短尾迹；颗粒从模型表面出现，不使用地面爆点。Q 的材质、Niagara 源资源与运行时配置均未修改。

### 实现要点

`FArrivalMaterialSnapshot` 分别保存 `PierceEffect` 与 `SurfaceEffect`，两者通过同一个局部采样器创建流程绑定 Static/Skeletal Mesh。材质仍是每槽隔离 MID。贯穿发射按当前帧与命中窗口的重叠时长分配，避免长帧直接越过整个短喷窗口；终结发射由既有 Arrival 阶段剩余时间推导。

最终溶解将 `RadiantStress` 置 0，使用 Q 原有的表面热光/边缘路径，`RadiantCritical` 保持 0。无需改写共享着色器，也不引入新的技能阶段、动画通知或目标类型分支。角色、骨架、动作、镜头、目标选择和伤害/完成契约保持原实现。

### 本版验证

- Development Editor 完整编译通过。小白人和大矿石近景连续捕获确认：贯穿喷散时实体仍在；最后的剩余表面持续白热，白色粒子跟随身体缺失过程出现。
- 逐帧捕获产生较长帧时，仍能看见贯穿红色喷散；GPU SimCache 同时确认三个贯穿发射器产生了实际粒子。
- 六目标完整释放并回到 Idle，镜头、移动和视角输入恢复。
- 分别在贯穿和白色溶解期间取消、再次释放；9 个受影响网格恢复精确原材质引用，两套表面采样器均无活动残留。
- 最终编译后运行 `Automation RunTests MineLearning.Guren`，Q 的预约生命周期、材质保真、掌心特效生命周期、表面覆盖以及 R 生命周期共 5 项全部通过。

在当前测试场景 Play 后按 R 即可复查。参数调整都在 `UltimatePresentation`，无需修改 Q。临时捕获与检查脚本不进入正式交付。

### 静态目标粒子轮廓修复

`NS_RadiantDissolve_Test` 和 `NS_ArrivalDisintegrate_Static` 的 `User.TargetMesh` 原配置为 `Default Mesh Only`，即使 R 已传入真实目标组件，Niagara 仍忽略它而采样测试矿石，且无法使用目标组件的世界变换。因此仓库、不同大小矿石等会出现相似的小团粒子。

两处采样模式改为 `Default`：优先使用显式绑定的目标组件及其网格、旋转、缩放，未绑定时仍保留资源编辑器的预览网格。只修正来源配置，未更改发射节拍、颜色、材质、速度、骨骼采样、技能或镜头代码。共享白色系统在 Q 中也会正确使用其已绑定的静态目标。

后台真实 RHI 验证：将测试方块旋转并缩放为 `(3, 1.5, 2)`，读取 GPU SimCache 后逆变换到目标局部空间；白色系统 1800 个粒子和贯穿系统 990 个粒子均贴近实际方块表面，并覆盖三个轴的两侧，未继续采样默认矿石。

测试场景完整 R 回归确认矿石、仓库、售卖机及 Gunner 配件的采样源指向实际目标；结算画面中的白色散解跟随各自形体，结束后移动和视角控制恢复。验证使用后台 UE 脚本与渲染捕获，不需要桌面输入。
