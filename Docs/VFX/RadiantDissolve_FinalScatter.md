# 红莲 Q：接触升温与末段散解（V3）

2026-09-18。正式资源位于 `/Game/MineLearning/VFX/RadiantDissolve/`，运行时不依赖 Python。V3 替代早期快速全身白热、大量白色粒子的配置；掌心六层特效、模型发光与局部补光见 [Q 配置说明](../Guren_Q_Configuration.md)。

## 视觉过程

接触点先泛深红，再向周围扩散红热；目标保留原来的底色、纹理与表面明暗。前 70% 不进行几何裁切，末段才出现局部白热和散解。粒子短暂向外飞散、缩小淡出，在 Q 结束前消退。不保留旧 Fresnel 空心轮廓。

`Dissolve(Target, OriginWS)` 接口保持不变。控制器在开始后以 Keep World 附着到目标，热源从原接触位置跟随目标运动；重置时先解除附着。不为矿石、机器人或仓库写目标类型特例。

## 配置

停止 Play，打开 **BP_RadiantDissolve_Test → Class Defaults → Radiant Dissolve**，修改后 Compile、Save。Q 每次生成新的控制器并采用这些默认值；只修改关卡里某个控制器实例不会修改 Q 的默认配置。

| 参数 | V3 默认值 | 用途 |
| --- | ---: | --- |
| Radiant Sweep End | 0.82 | 热前沿展开完成比例 |
| Radiant Dissolve Start | 0.72 | 开始裁切表面 |
| Radiant Mesh Gone | 0.98 | 表面完全隐藏 |
| Radiant Particle Start | 0.72 | 开始生成散解粒子 |
| Radiant Particle End | 0.89 | 停止生成，留出尾段淡出时间 |
| Radiant Particle Inward Depth | 10 cm | 表面向内随机采样深度 |
| Radiant Particle Rate | 2400/s | 多个 Mesh 平分的总采样率 |
| Radiant Particle Speed | 45 cm/s | 向外散开的速度基准 |
| Radiant Heat Intensity | 6 | 目标表面热发光强度 |
| Radiant Noise Strength | 12 cm | 前沿位置扰动 |
| Radiant Edge Width | 3 cm | 散解边缘过渡 |
| Radiant Duration | 2 s | 独立演示默认时长；Q 按动画通知覆盖 |

0–1 的参数是总时长比例。允许热前沿与末段溶解重叠，不再沿用旧版 Sweep End 必须早于 Dissolve Start 的限制。应保持 `Dissolve Start < Mesh Gone ≤ 1`，并保证 `Particle End + 最大粒子寿命比例 < 1`。

两个 GPU 系统 `NS_RadiantDissolve_Test` 和 `NS_RadiantDissolve_Skeletal` 的 SurfaceEnergy / Particle Spawn / Set Parameters 写入实际 `Particles.Lifetime`：

```hlsl
User.EffectDuration * (0.06 + 0.04 * frac(sin(float(Particles.UniqueID) * 37.719 + 17.17) * 43758.5453))
```

寿命为总时长的 6%–10%，最迟在 99% 时自然消退。不要只改 Initialize Particle 中的备用寿命值；后续 Set Parameters 会覆盖它。Q 当前辐射时长约 2.8667 s：2.064 s 开始散解，2.551 s 停止发射，粒子寿命约 0.172–0.287 s，最迟约 2.838 s 消退。

Q 的 `RadiationChanged` 从 `GetRadiationDuration()` 取得 StartDissolve → DissolveFinish 的通知间隔，并关闭演示用 Origin Marker。无需在蓝图中另写动画秒数。

## 代码与材质职责

- **BP_RadiantDissolve_Test**：收集兼容 Mesh，保存每槽原材质，通过通用 CreateIsolatedMaterialInstance 创建原着色器的临时 MID 和对应 GPU sampler，推进统一进度、热前沿和粒子窗口；负责材质、可见性与粒子清理。
- **MF_RadiantDissolve**：保留带噪声的世界空间前沿距离，输出红热权重、局部边缘和表面遮罩。前段热权重较低，随进度增强；到 1 时遮罩明确归零。
- **MF_RadiantSurface**：将原始 Base Color、Emissive、Opacity Mask 接入共享热变色/溶解结果；原有 Normal、Metallic、Roughness、WPO 等保持原连接。Q 不再按材质名称选择 M_RadiantDissolve_*Surface 替代着色器。
- **两个 NS_RadiantDissolve 系统**：沿静态/骨骼表面采样，向内随机偏移并向外散开。两者使用相同寿命比例，统一结束时序。
- **GurenQPresentationComponent**：在 Radiation 前快照目标材质与显隐，释放/取消时恢复，并通知控制器停止。掌心自己的 MID、Niagara 与局部灯光另由同一表现组件管理。

## 验证与适用范围

V3 在实际 Q 中检查了 Gunner、OreBuddy、大矿石和仓库，包含正面、侧面、斜面、关闭 Bloom、折射开关以及辐射中取消。检查随目标移动的接触热源、目标前段形体可读性及末段散解；最终结果同时记录于 Q 配置说明的 V3 验收部分。

向内采样是近似表面厚度层，不是封闭模型的精确体积或 SDF 求解。薄模型应减小 Inward Depth。未新增 ISM/HISM、Geometry Collection 或任意未知主材质运行时自动注入支持。未接入材质函数的目标仍保持其原材质，不再被强制换成灰白通用材质；如需表面红热/裁切，应按以下方式接入。

修改后应复验：完整 Q → 取消 Q → 再次 Q，确认目标材质/显隐恢复、无残留控制器或粒子，并检查最晚出生的粒子在目标销毁前淡出。该共享控制器的其他使用者也会采用 V3 默认节奏。

## 材质丢失修复与接入规范（2026-09-18）

旧实现只对 Gunner/AK 保留专用替代着色器，其他目标统一换成 M_RadiantDissolve_Surface，再复制参数。这无法保留 OreBuddy 的 DiffuseColor、矿石的直接纹理连接和其他原始着色逻辑，导致抓取后变成灰白模型。前一轮只检查了粒子和清理，未充分检查目标外观，此项验收由本轮材质保真回归补充。

现在 Static/Skeletal 的材质准备走同一函数 `UMaterialEffectLibrary::CreateIsolatedMaterialInstance`。原材质图不被替换：保留常量实例父级（包括静态开关），复制当前有效运行时参数到独立 MID，结束后恢复原材质对象。不能直接调用 Mesh 的 CreateDynamicMaterialInstance 代替它：该 API 会复用已有 MID，从而把溶解参数写进履带、屏幕等业务材质。

新材质只需一次性接入 `/Game/MineLearning/VFX/RadiantDissolve/MF_RadiantSurface`：

1. 原 Base Color 接 OriginalBaseColor，原 Emissive 接 OriginalEmissive；两个输出分别接回对应通道。
2. 原来为 Masked 的材质，将原 Opacity Mask 接 OriginalOpacityMask，输出接回 Opacity Mask；原来为 Opaque 的材质使用默认 1，并设为 Masked。不要把原先被 Opaque 忽略的失效 Mask 接入。
3. 保留所有其余通道、UV、纹理采样、实例参数和静态开关。函数默认 DissolveProgress=0，输出原颜色/原发光/原遮罩，日常外观不改变。
4. 控制器只更新统一的 DissolveProgress、DissolveOriginWS、HeatRadius、DissolveRadius 等参数，无需增加 Actor 类型判断或材质名称白名单。

接入检查必须覆盖运行时会切换的全部材质，不能只检查初始网格。矿石的 `OreVisualComponent.Stages` 会在采矿时切换网格和 MaterialOverride，100/80/60/40/20 五个阶段均须支持共享函数；Quinn 的两个实例共用 `M_Mannequin`。旧材质还可能保存 `bCanMaskedBeAssumedOpaque` 优化标志：新增动态 Opacity Mask 后，实际 `GetBlendMode()` 必须是 Masked，仅看到编辑器 Blend Mode 为 Masked 不足以验收。

还需检查材质根输入的 `UseConstant`。UE 的 `FColorMaterialInput` / `FScalarMaterialInput` 在该标志启用时优先编译内联常量，即使已经连上函数输出也会忽略连接。完整矿石的 Emissive 和 Opacity Mask 因此分别保持旧发光及不透明，表现为参数正常推进但模型不红热、不裁切。接入时将原内联颜色/发光/遮罩值转换为常量节点，接入函数对应 Original 输入，再关闭根输入的 UseConstant；不要直接清除常量而丢掉原外观。本项目已接入的主材质统一处理此项，不修改 Nanite 设置，也不增加目标类别分支。

OreBuddy 和仓库的导入实例原来继承引擎插件的 FBXLegacyPhongSurfaceMaterial。为避免修改引擎资源，项目保留完整着色图副本 `M_RadiantCompatiblePhong` 并接入同一函数；对应实例仅调整父级，原向量、标量、贴图值不变。该副本的原始来源为 `/InterchangeAssets/Materials/FBXLegacyPhongSurfaceMaterial`（UE 5.8）。项目已有的矿石、履带、工具金属、Gunner/AK、仓库和售卖机材质均保留原着色逻辑。

`RadiantReset` 恢复原材质后不再调用 `RadiantApplyFrame`，避免将溶解参数写回业务 MID。`RadiantEnablePreview` 在没有材质快照时先执行 Prepare；`RadiantPreviewAt` 通过同一入口应用预览，保证预览也只写临时实例。

回归入口增加 `MineLearning.GurenQ.MaterialPreservation`，检查原始材质父级、DiffuseColor、动态标量、原 MID 隔离、重复执行、零溶解进度及非法槽位。

本轮最终验证：

- UE 5.8 Development Editor 编译、链接成功；`GrabbableLifecycle`、`MaterialPreservation`、`PalmRadiationLifecycle` 三项自动化测试全部通过。
- 实际 PIE 分别抓取 OreBuddy、矿石、大矿石、仓库、售卖机和 Gunner，在辐射阶段取消；再对同一 OreBuddy 完整执行一次，以 Restore 完成策略检查恢复。每次均断言技能实际目标是指定对象。
- 七组共检查 55 次材质槽：辐射中原着色器、已有向量/标量/贴图参数保留；结束后恢复同一个原材质对象，不新增原 MID 参数覆写；掌心实例归零。最终运行无 Blueprint Runtime Error。
- 固定同一动画姿势、相机和照明，对照原材质与零进度临时材质；人工检查 OreBuddy 黄色外壳、深色底盘和履带细节。另检查矿石纹理、仓库蓝色面板、售卖机配色，以及末段散解仍然生效。

## 高亮与裁切补验（2026-09-18）

上一轮材质保真检查不能证明目标的高亮与裁切有效。本轮补齐 Quinn 和矿石阶段材质，并修正上述根输入内联常量及旧不透明优化状态。制作时使用的一次性资产修正工具已移除；运行时仍只使用原来的统一控制器和材质接口。

新增 `MineLearning.GurenQ.RadiantSurfaceCoverage`，检查 Quinn 两个实例、矿石 100/80/60/40/20、两种掉落矿石及受损材质：实际混合模式、共享参数、Emissive/Opacity Mask 连接及 UseConstant 状态。最终 Development Editor 编译成功，四项 `MineLearning.GurenQ` 自动化测试全部通过。

实际 Q 进入 Radiation 后，固定姿势，用独立 SceneCapture 仅捕获可见目标网格，排除血条、背景和掌心粒子。下表是 192×192 渲染目标中的可见表面像素数；不以 Actor 被销毁代替材质裁切验收。

| 场景目标 | 原始表面 | 65% 升温 | 82% 分解 | 88% 分解 | 99% 末段 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Quinn 小白人 | 402 | 402 | 263 | 127 | 0 |
| 矿石 | 532 | 532 | 495 | 11 | 0 |
| 大矿石 | 954 | 954 | 809 | 136 | 0 |
| OreBuddy | 595 | 595 | 487 | 70 | 0 |
| 仓库 | 1737 | 1737 | 1731 | 994 | 0 |

售卖机和 Gunner 同样通过升温、部分裁切、末段归零以及取消后原材质对象恢复检查。矿石还执行了四次伤害阶段后的重复抓取；该场景这四次运行仍使用 M_Ore_Iron_100，不把它们计作四种不同阶段材质的视觉验收。其他阶段资源由上述材质覆盖自动化检查。

另对小白人、矿石、大矿石完整播放 Q，不暂停、不调用 RadiantPreviewAt，确认真实动画通知推进、红热增强、末段表面归零及目标按原完成策略销毁。可见像素分别为 `402 → 402 → 216 → 0`、`532 → 532 → 298 → 0`、`954 → 954 → 628 → 0`。最终两组验证运行无 Blueprint Runtime Error。测试只在临时 PIE 中关闭选目标的视线限制以隔离场景遮挡，不修改正式选择配置；本轮不将该项作为视线选择测试。
