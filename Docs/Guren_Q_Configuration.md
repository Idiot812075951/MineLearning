# 红莲 Q 技能：配置与调试说明

当前实现日期：2026-09-14。此文档以当前 UE 资产和代码为准，取代旧交付说明中“抓头时震屏”“结束时再震一次”的描述。

## 当前播放顺序

Q 选取最近测试目标 → 远距离动态突进 / 近距离直接抓取 → **GrabContact 接触：短慢镜头，同时开始绕到红莲正面** → 提起 → **StartDissolve：启动辐射溶解，同时震屏一次** → 目标消失 → 镜头回到抓取前视角 → 恢复正常控制。

震屏不会在抓取接触或溶解结束时额外触发。镜头保持在正面稍偏侧，减少被抓目标遮挡红莲头胸的情况。绕行使用现有 SpringArm 的碰撞检测；不会另建 PlayerController。鼠标转向在处决镜头控制期间暂时锁定，结束和取消均恢复；离场则立即恢复。

## 1. 最常用配置入口

在 Content Browser 打开：

`/Game/MineLearning/Characters/Guren/Blueprints/BPC_GurenQPresentation`

点击 **Class Defaults**，搜索以下分类。修改后 Compile、Save，并重新 Play。此蓝图组件挂在 `BP_GurenRetargetTest` 的 `QPresentation` 上；如果组件实例有覆写，实例值优先于类默认值，可用黄色重置箭头恢复继承。

### Q Execution Camera：正面处决镜头

| 参数（代码名） | 当前默认值 | 含义 |
|---|---:|---|
| Use Execution Camera (`bUseExecutionCamera`) | 开启 | 关闭后不自动绕到正面；震屏、慢镜头和 FOV 效果仍独立工作 |
| Execution Camera Distance | 750 cm | 处决时 SpringArm 的目标距离；变大能容纳更多身体和翅膀，遇墙时碰撞检测可能缩短实际距离 |
| Execution Camera Yaw Offset | -20° | 相对“红莲正对面”的偏转。0 为正面；正负值选择另一侧。当前略偏侧以避开敌人遮挡 |
| Execution Camera Pitch | -10° | 摄像机俯仰；负数俯视，增大趋向平视 |
| Execution Camera Height | +60 cm | 在原 SpringArm TargetOffset 上增加的世界 Z 高度，抬高构图中心以容纳举起的敌人 |
| Execution Camera Blend In | 0.7 s | 从 GrabContact 开始绕到正面的真实时间；使用平滑插值，不受慢镜头延长 |
| Execution Camera Blend Out | 0.45 s | 溶解完成或取消后回到原视角的真实时间 |

实际控制旋转为：`Pitch = 配置俯仰；Yaw = 红莲朝向 + 180° + YawOffset`。SpringArm 位于视线后方，因此相机位置落在红莲前方。镜头旋转不会旋转红莲模型。恢复的是 GrabContact 时保存的控制旋转、滚轮对应的臂长及原 TargetOffset；处决镜头期间滚轮不用于改变处决构图。

推荐调整顺序：先 Distance → Height → YawOffset → Pitch，最后调整过渡时间。目标挡脸时优先改 YawOffset；敌人头出画面时优先增加 Height 或 Distance；不建议通过改骨骼或抓取 Socket 来修镜头构图。

### Q Camera：震屏、慢镜头与拉近

| 参数 | 当前默认值 | 含义 |
|---|---:|---|
| Impact Strength | 1 | **只在 StartDissolve** 使用的震屏强度，0 关闭，面板范围 0–2 |
| Dissolve Shake Duration | 0.35 s | 溶解开始的震屏衰减时间，真实秒，0 关闭 |
| Close Up FOV | 10° | 相对基础 FOV 的缩小量；Grab 使用 65%，Radiation 使用全部。基础 90 时约为 83.5 / 80；0 关闭此拉近 |
| Contact Time Scale | 0.25 | GrabContact 时短慢镜头倍率，1 为正常速度；乘在进入前的全局时间倍率上 |
| Contact Slow Motion Duration | 0.22 s | 抓取慢镜头的真实持续时间，0 关闭；结束/打断恢复进入前倍率 |

慢镜头仍在抓住敌人的接触时触发，本轮只改变震屏时机。采用单机 Demo 的全局时间缩放，因此环境动画/物理也一起减速。镜头插值、震屏衰减和恢复使用真实时间。

### Q Animation / Q Effects：资源装配

| 参数 | 当前资源 |
|---|---|
| Dash Montage | `/Game/MineLearning/Characters/Guren/Animations/Q/AM_Q_Dash` |
| Grab Montage | `/Game/MineLearning/Characters/Guren/Animations/Q/AM_Q_GrabDissolve` |
| Afterimage Material | `/Game/MineLearning/Characters/Guren/VFX/M_QDashAfterimage` |

动画、特效与镜头表现由该组件订阅 Q 技能事件；不要把镜头逻辑接到角色移动 Tick 或飞行蓝图上。

## 2. 施法距离、目标与抓取位置

打开 `/Game/MineLearning/Characters/Guren/Blueprints/BP_GurenRetargetTest`，选中继承的 **QSkill** 组件，在 **Q Skill** 分类调整：

| 参数 | 当前值 | 含义 |
|---|---:|---|
| Selection Range | 2000 cm | 最大目标搜索距离，按水平距离；选择最近的 `BP_QGrabTestDummy` |
| Direct Grab Range | 200 cm | 小于等于此值直接抓取，超过则突进；应小于最大距离 |
| Grip Socket | `Q_GrabHead` | 右手抓头的挂点名称，必须与当前 Skeleton 一致 |

单位是 UE cm；“500–1000 码”只作为玩家要求的游戏身位感参考，不是现实码数换算。Q 输入在 `Source/MineLearning/Manifestation/Guren/AGurenCharacter.cpp` 的 `SetupPlayerInputComponent` 中绑定 `EKeys::Q`，当前没有独立 Q 的 Enhanced Input 资产。

目标蓝图：`/Game/MineLearning/Characters/Guren/Blueprints/BP_QGrabTestDummy`。

- `GrabStandPoint`：抓取的参考站位，当前相对目标头部根节点为 `(-165, -53, -298)` cm，旋转为 0。可在蓝图中改或对场景实例覆写。R19 开始会根据“红莲 → 目标”的水平接近方向，将参考朝向与偏移一起绕目标旋转，保证目标位于角色右前方；它不再把所有接近方向强制到目标的固定朝向。站位 Z 使用施法时角色高度。
- Dummy Root 是头部中心，Body 网格做相对偏移。当前使用 Quinn Simple，比例约 `1.74136`；这些值按当前抓取动画校准，换敌人尺寸应一起校准头部与站位。
- Skeleton：`/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren_Skeleton`；`Q_GrabHead` 属于 `radiant_hand_r`，相对位置约 `(-9.99025, -46.15375, -36.04199)` cm，旋转 0、比例 1。
- 抓取时根节点吸附 Socket，保留世界旋转和比例，使小白人垂直悬挂；禁用其碰撞，避免推开红莲。

不要为了调站位修改 Skeleton 层级或源动画 Rest Pose。当前只搜索测试 Dummy，不是任意敌人通用锁定系统；没有正式伤害、血量或联网逻辑。

## 3. Montage、Root Motion 与事件时机

两段动画在 `/Game/MineLearning/Characters/Guren/Animations/Q/`：

- `AN_Guren_Q_Dash`：30 FPS，约 0.5667 s；启用 Root Motion，Force Root Lock 关闭。
- `AN_Guren_Q_GrabHeadIK`：30 FPS，约 4.0667 s；Root Motion 关闭，Force Root Lock 开启。
- AnimBP：`/Game/MineLearning/Characters/Guren/Blueprints/ABP_GurenLocomotion`，使用 **Root Motion From Montages Only**；**QFullBody** Slot 位于普通攻击下半身混合之后，Q 是全身动作。

在 Montage 的 **Q Events** 通知轨中修改时机。下表秒数是素材播放时间，不是包含慢镜头之后的现实时间。

| Montage / 通知 | 时间 | 作用 |
|---|---:|---|
| AM_Q_Dash / Motion Warping 窗口 | 0.1333–0.55 s | SkewWarp；Target=`Q_DashTarget`，平移/旋转开启，忽略 Z |
| AM_Q_Dash / DashArrival | 0.55 s | 下一帧验证 Warp 终点，然后衔接 Grab |
| AM_Q_GrabDissolve / GrabContact | 0.20 s | 扣头、挂接目标；短慢镜头和正面镜头开始 |
| AM_Q_GrabDissolve / StartDissolve | 1.0667 s | 开始辐射溶解，**震屏触发点** |
| AM_Q_GrabDissolve / DissolveFinish | 3.9333 s | 清理效果、删除目标、开始恢复镜头 |
| AM_Q_GrabDissolve / SkillEnd | 4.0333 s | 结束技能、恢复移动控制 |

通知类是 `Guren Q Event`，其 Event 字段必须与上表拼写一致。提前/延后震屏应移动 `StartDissolve`，它同时驱动溶解，二者不会用独立 Delay 猜时间。

Dash Blend In/Out 为 0.06/0.05 s；Grab 为 0.08/0.15 s。Dash 播放时间按距离计算：`Clamp(0.48 + 目标距离/3500, 0.57, 1.05)` 秒，再换算 Montage 播放倍率。角色真实突进由动画 Root Motion 和 Motion Warping 驱动，非整个 Mesh 独立平移。

如果延长辐射段，必须同时调整：源动画/Grab Montage 长度、StartDissolve/DissolveFinish/SkillEnd 通知、下面的 RadiantDuration，并检查 8 秒保护超时是否仍足够。

## 4. 溶解效果配置

`BPC_GurenQPresentation → Event Graph → RadiationChanged` 创建已有的 `/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test`，调用 **Dissolve(Target, OriginWS)**。Origin 来自 `Q_GrabHead` 世界位置。

Q 运行时在调用前明确覆盖两个值：

| 参数 | Q 中实际值 | 修改位置 |
|---|---:|---|
| RadiantDuration | 2.866667 s | BPC_GurenQPresentation 图中的 Set Radiant Duration 节点；应等于 DissolveFinish − StartDissolve |
| RadiantShowOriginMarker | false | 同图 Set Radiant Show Origin Marker 节点；正常游戏不显示调试球 |

因此只修改原溶解蓝图的这两个类默认值不会改变 Q 的实际值。其余外观参数继承 `BP_RadiantDissolve_Test → Class Defaults → Radiant Dissolve`：

| 参数 | 当前类默认值 | 作用 |
|---|---:|---|
| RadiantNoiseStrength | 12 cm | 传播边缘不规则程度 |
| RadiantHeatIntensity | 65 | 全身表面的白热亮扫强度 |
| RadiantEdgeWidth | 5 cm | 转换边缘宽度 |
| RadiantSweepEnd | 0.30 | 全身亮扫完成比例 |
| RadiantDissolveStart / RadiantMeshGone | 0.34 / 0.52 | 表面开始转换 / 完全隐藏 |
| RadiantParticleRate | 18000/s | 密集内部层粒子总采样率 |
| RadiantParticleStart / RadiantParticleEnd | 0.34 / 0.57 | 开始 / 停止生成的归一化时机 |
| RadiantParticleInwardDepth | 10 cm | 随机沿表面法线向内取样深度 |
| RadiantParticleSpeed | 85 cm/s | 粒子向外扩散速度 |

改这些共享类默认值会影响使用该蓝图的其他测试。只想给 Q 单独调时，在 Q 的 `RadiationChanged` 图中、调用 Dissolve 前增加相应 Set 节点。2026-09-15 已取消空心轮廓停留，改为亮扫与白色粒子散解，详见 [当前配置说明](VFX/RadiantDissolve_FinalScatter.md)。

取消时会停止特效、恢复可见性和本次替换前的原材质、解除绑定及恢复目标原位置。正常完成后删除目标。

## 5. 残影材质与代码固定值

`M_QDashAfterimage` 为 Additive / Unlit 材质，`TrailColor` 默认线性色 `(0.85, 0.008, 0.11)`。可创建 Material Instance 调色，再指定到 Afterimage Material。`Opacity` 默认 0.24，但运行时每帧会覆写，因此只改材质参数默认值不会改变当前残影淡出强度。

以下目前是 C++ 内部值，**没有面板配置项**；修改后需编译。集中在 `GurenQPresentationComponent.cpp`：

| 项目 | 当前值 |
|---|---|
| 残影开始等待 | Dash 开始后 0.12 s，且水平速度超过 300 cm/s |
| 残影间隔 / 生命周期 / 同时数量 | 0.065 s / 0.24 s / 最多 4 |
| 残影透明度 | `0.24 × 剩余寿命比例²` |
| 残影碰撞 / 阴影 | 均关闭 |
| 冲刺广角 | 基础 FOV +4° |
| Grab 拉近系数 | CloseUpFOV × 0.65 |
| FOV 插值速度 | 7，基于真实 Delta |
| 震动频率 | 正弦角频率 150 rad/s，约 23.9 Hz |
| 震动角度比例 | Pitch 0.8°、Yaw 0.45°、Roll 0.25°，乘 ImpactStrength 和衰减包络 |
| 震动平移比例 | 相机局部 Y 1.5 cm、Z 1 cm，同样乘强度与包络 |
| 震动包络 | 剩余时间比例平方，逐渐减弱 |

`GurenQSkillComponent.cpp` 固定保护值：技能总超时 8 s、Dash 终点误差允许 55 cm、抓头接触误差允许 100 cm、Grab 接触前站位插值速度 25。正常技能事件负责结束，超时只处理异常。技能中重复 Q 忽略，普通攻击及飞行受互斥控制。

## 6. 地图和使用方式

地图：`/Game/MineLearning/Maps/L_Guren_Retarget_Test`。三个小白人在 Outliner 的 **Q Skill Targets** 文件夹；出生点附近的目标约 1000 cm。其他标签表示长距离或近抓测试站点，不表示与任意玩家当前位置的实时距离。

停止 Play 后改参数并保存，再 Play 测试。重启 Play 会恢复被删除的目标。近距离测试先走到 200 cm 内；远距离走到 200–2000 cm；不要同时把多个目标摆在搜索范围内靠得太近，否则 Q 会按最近目标选择。

建议先看正常完整处决，再测试抓取中取消和溶解中取消。检查镜头回转、鼠标转向恢复、时间倍率恢复和原滚轮距离。源码位置：`Source/MineLearning/Manifestation/Guren/GurenQPresentationComponent.h/.cpp` 与 `GurenQSkillComponent.h/.cpp`。一次性 Python 施工和验证文件已清理，运行时不依赖 Python。

本轮验证：完整执行时采集到的震动只属于 Radiation；绕行中取消没有震动，溶解开始时取消会立即停止震动。三种路径均恢复测试前控制旋转 `(-18°,25°,0°)`、臂长 680 cm、TargetOffset `(5,9,12)`、FOV 90、时间倍率 1，移动与鼠标转向解锁，表现 Tick 停止。这里只保留既有验证结论，原始检查记录已清理。已在当前地图观察正面提起与溶解构图。
