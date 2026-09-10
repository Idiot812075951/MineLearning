# 辐射分解 VFX：使用、调试与计算说明

> 当前状态：**未完成**。这是可验收的 VFX Prototype，还不是可直接交付正式技能的最终溶解功能。

这是独立的表现控制器。它接管目标 Actor 及其递归 Attached Actor 中当前可见的普通 `StaticMeshComponent` 与 `SkeletalMeshComponent`，从指定世界点依次表现红色透明轮廓、表面红热、不规则分解、轮廓停留与淡出、白色粒子残留。它不包含抓取、攻击、伤害或 Actor 销毁逻辑。

## 快速测试

打开 `/Game/MineLearning/VFX/RadiantDissolve/L_RadiantDissolve_Test`。控制器资产是 `/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test`。外部只有一个正式播放入口：

`Dissolve(Target, OriginWS)`

`Target` 是场景 Actor，`OriginWS` 是掌心接触点的世界坐标。调用方不判断 Static Mesh 或 Skeletal Mesh，不选择 Niagara System，也不选择 Gunner、AK 或其他兼容材质。再次调用会先完整 Reset 再从新目标和新起点播放；需要主动复原时调用 `RadiantReset`。

## 在其他 Actor 上使用

1. 在场景放置 `BP_RadiantDissolve_Test`。
2. 从抓取或交互逻辑调用 `Dissolve(Target, OriginWS)`。
3. 测试地图手工预览时，也可以在 Details 指定 `RadiantTarget`，让 `DissolveOrigin` Scene Component 表示接触点，再调用内部播放函数。
4. 调用 `RadiantReset` 恢复目标。

目标不需要继承接口。控制器会枚举目标和递归 Attached Actor 的 Static/Skeletal Mesh 组件与全部材质槽，只处理调用时可见的组件，保存每个源材质，创建动态材质实例，并在 Reset 时按原顺序精确恢复。隐藏的 Camera Proxy、备用弹匣等不会被误激活。

Actor 的组件、Bounds、Origin、计时、Reset 和表面采样走同一公开流程。内部会枚举完整附着层级的 Mesh Component 和全部材质槽，并选择引擎要求的数据接口。调用方看不到网格类型分支。

材质有一个 UE 本身的边界：运行时不能向已经编译完成的任意 Master Material 注入 Masked 像素裁切。正式资产的 Master Material 需要预先接入 `MF_RadiantDissolve`；当前 Demo 的旧材质通过内部兼容层接入。兼容材质、源材质检测和 Niagara 资产引用已经全部隐藏，不再是实例参数。播放时复制源 Material Instance 参数，Reset 时按原槽位恢复，因此不会出现 Gunner 白膜。

## Origin 与可视化

自动点计算为：

`Origin = BoundsCenter + (0, 0, BoundsExtentZ * RadiantOriginHeightRatio)`

类默认值是 `0.22`。测试地图实例保留了当前调好的 `0.596`，所以标记位于模型偏上的区域。粉红小球 `OriginMarker` 显示最终世界坐标，由 `RadiantShowOriginMarker` 控制。正式技能接入时，把 `DissolveOriginWS` 更新为红莲右掌与目标的接触点即可。

## 参数在哪里调

最方便的位置是测试地图里 `BP_RadiantDissolve_Test` 实例的 Details → `Radiant Dissolve`。修改类默认值则打开 `/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test`。这些是 `.uasset` 内的蓝图变量，不是文本配置文件。

| 参数 | 默认值 | 作用 |
|---|---:|---|
| RadiantShowOriginMarker | true | 显示粉红 Origin 标记 |
| RadiantDuration | 2.4 s | 完整 Mesh 阶段时长 |
| RadiantNoiseStrength | 12 cm | 扰乱世界空间传播边界；内部限制到有效半径的 22% |
| RadiantHeatIntensity | 88 | 红热、粉白分解边和轮廓亮度 |
| RadiantEdgeWidth | 4 cm | 高热分解边宽度 |
| RadiantParticleRate | 7500/s | 白点候选采样率；Noise/Front/内区概率会再次过滤 |
| RadiantParticleStart | 0.84 | 此相位以前强制 Spawn Rate 为 0 |
| RadiantParticleSpeed | 36 cm/s | 白点统一向外扩散速度 |

Target 与 Origin 只存在于 `Dissolve(Target, OriginWS)` 的函数输入，不在 Details 重复配置。面板只显示8个结果外观或调试参数。想让白点更多，增大 `RadiantParticleRate`；想让白点出现更晚，提高 `RadiantParticleStart`；想让白点飞得更远，提高 `RadiantParticleSpeed`。阶段分界、覆盖距离、材质引用、网格类型数组和 Niagara 引用固定为内部数据。

## 顺序与材质计算

每个像素先计算带噪声的世界空间距离：

`d = length(AbsoluteWorldPosition - DissolveOriginWS) + SpatialNoise(WorldPosition)`

`HeatRadius` 从第 0 帧开始增长；`DissolveRadius` 在归一化时间 `0.24` 后才从负安全半径增长。因此先看到原材质上的红边和接触区红热，再开始真正裁切。到 `0.66` 时分解传播已完成约 92%，余下的是远端不规则碎片和 Fresnel 轮廓；到 `0.84` 才开始用 Masked dither 淡出轮廓，最后在 `0.96` 归零。

Heat 区域保留源 Base Color，通过附加少量暖色和 Emissive 表示升温，避免角色在分解前变成黑膜。分解使用 Masked Opacity；`DissolveEdgeWidth` 计算粉红/白热边。Noise 使用世界坐标锚定，所以不会退化为脚底向上或完美球面。Reset 时先恢复源材质和可见性，再把时间置 0；传播距离或 Noise 调大不会改变初始完整状态。

自动覆盖距离为：

`Coverage = max(distance(ComponentCenter, Origin) + length(BoundsExtent) + margin)`

它按所有目标组件取最大值，因此不同尺寸的枪、矿石和角色不需要手填最终半径。

## 白色粒子计算

统一控制器会根据目标 Mesh Component 自动绑定 UE 所要求的表面数据接口；调用方始终只传 Target 和 Origin。粒子采样当前网格表面，骨骼动画姿态也会实时参与采样。控制器统一写入 Origin、Dissolve Radius、Noise、Front Width、Speed 与 Spawn Rate。

候选点使用与材质相同的带噪声距离。系统保留当前分解前沿，并从已经分解的内区再随机保留约 5.5%，所以白点不会只贴着一圈边界。`RadiantParticleStart` 以前 Spawn Rate 恒为 0；默认到轮廓开始淡出才出现白点。速度始终以 `normalize(ParticlePosition - OriginWS)` 为正方向，再叠加小于主速度的横向扰动与轻微上浮，不存在向内随机分支。粒子尺寸为约 `0.55～1.35`，生命周期约 `0.72～1.10 s`，末段缩小和淡出。

## 蓝图执行流程

`Dissolve(Target, OriginWS)` 的执行顺序：

1. 保存统一的 Target 和 Origin，进入 `RadiantPlay`。
2. `RadiantReset` 清理旧粒子并恢复原材质。
3. `RadiantResolve` 收集目标与递归 Attached Actor 中当前可见的 Mesh Component。
4. `RadiantPlaceOrigin` 放置接触点；`RadiantComputeCoverage` 从 Bounds 推导覆盖半径。
5. 内部保存源材质、创建 MID、复制材质实例参数并绑定表面粒子采样。
6. 1/60 秒计时器调用 `RadiantApplyFrame`，统一更新材质与 Niagara 参数。
7. `RadiantFinish` 停止发射并隐藏本次收集到的全部 Mesh，保证武器、弹匣和身体同时结束；`RadiantReset` 恢复它们的原材质与可见性。

逻辑按解析、材质准备、恢复、表面粒子和单帧参数拆成短函数，没有 Tick 和无限延伸的 Event Graph。网格类型差异仅存在于引擎数据接口绑定处，不进入公开 API。

## 定格调试

需要检查具体相位时使用内部 `RadiantPreviewAt(Phase)`，正式调用不依赖该函数：

1. 调用 `RadiantPlay` 创建本次材质和粒子实例。
2. 调用 `RadiantPreviewAt(Phase)`，Phase 范围为 `0～1`。
3. 调用 `RadiantReset` 返回完整状态。

建议检查 `0.15`（原材质 + 红轮廓）、`0.45`（接触点向外分解）、`0.70`（不规则残留轮廓且没有白点）、`0.85`（轮廓淡出并开始生成白点）和 `0.96`（仅残余白点）。

## 后续接 Skeletal Mesh

控制器已经支持 Skeletal Mesh Component，无需改调用和传播计算。新的角色材质体系只需在内容制作阶段把 `MF_RadiantDissolve` 接入公共 Master Material；运行时仍然调用同一个 `Dissolve(Target, OriginWS)`。骨架、动画、蒙皮、Rest Pose 和 Skeletal Mesh 资产都不用改。

当前会递归处理附着 Actor/Child Actor 中的普通 Static/Skeletal Mesh。仍不处理 ISM/HISM 单实例、Geometry Collection、Procedural Mesh 或无 Mesh 的 Actor。

## 资产列表

资产均位于 `/Game/MineLearning/VFX/RadiantDissolve/`：

- 控制与地图：`BP_RadiantDissolve_Test`、`L_RadiantDissolve_Test`、`BP_RadiantSkeletalTargetSample`
- 核心材质：`MF_RadiantDissolve`、`M_RadiantDissolve_Surface`、`M_RadiantDissolve_Test`、`MI_RadiantDissolve_Test`
- 内部材质兼容资产：只为尚未接入公共 Material Function 的旧测试材质服务，不在控制器 Details 中暴露
- 表面粒子：控制器内部自动选择并绑定，外部没有 Static/Skeletal 分支
- Origin：`M_RadiantOriginMarker`

`Tools/RadiantDissolve` 只保存施工、回归和验证脚本，运行时不依赖 Python。
