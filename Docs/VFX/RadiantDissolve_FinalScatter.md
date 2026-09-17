# 红莲 Q：亮扫与白色粒子散解

2026-09-15 当前版本。替代旧的红色空心轮廓停留方案。源资产在 `/Game/MineLearning/VFX/RadiantDissolve/`；运行时不依赖 Python。

## 怎么调

停止 Play，打开 **BP_RadiantDissolve_Test → Class Defaults → Radiant Dissolve**。修改后 Compile、Save，再 Play。这里修改的默认值会被 Q 每次新生成的控制器采用；在地图里放一个同名 Actor 并修改它，只会影响该实例。

| 面板参数 | 当前值 | 作用 |
|---|---:|---|
| Radiant Sweep End | 0.30 | 全身表面亮扫完成时机 |
| Radiant Dissolve Start | 0.34 | 表面开始转换成粒子 |
| Radiant Mesh Gone | 0.52 | 剩余表面完全隐藏 |
| Radiant Particle Start | 0.34 | 密集粒子开始生成 |
| Radiant Particle End | 0.57 | 停止生成，让现存粒子继续散开 |
| Radiant Particle Inward Depth | 10 cm | 从表面随机向内采样的最大深度；0 为表面采样 |
| Radiant Particle Rate | 18000/s | 总采样率，多个 Mesh 平分；已取消旧的窄边缘/5.5%残留过滤 |
| Radiant Particle Speed | 85 cm/s | 从各 Mesh 中心向外散开的速度基准 |
| Radiant Heat Intensity | 65 | 整个受扫表面的白热强度 |
| Radiant Noise Strength | 12 cm | 扫描/转换前沿的扰动 |
| Radiant Edge Width | 5 cm | 转换前沿的过渡范围 |
| Radiant Duration | 2 s | 独立控制器默认时长；Q 会覆盖此项 |

0–1 的时间参数都是总时长比例，不是秒。保持 `Sweep End ≤ Dissolve Start < Mesh Gone ≤ Particle End ≤ 0.60`，可保留充足的粒子消散时间。Depth 不宜盲目调大：它是从表面沿法线向内取不同深度的近似体积层，不是封闭模型的精确体积/SDF 求解。特别薄的模型应减小该值。

Q 在 `BPC_GurenQPresentation → Event Graph → RadiationChanged` 内仍覆盖 `RadiantDuration = 2.866667 s` 和 `RadiantShowOriginMarker = false`，其他值继承共享控制器默认值。只想对 Q 单独调时，在该事件的 `Dissolve` 调用前设置对应实例变量。

## 现在的过程

从抓头接触点沿全身亮扫；短暂全身白热后，表面转换为密集的小白色粒子，取消 Fresnel 残留壳层。骨骼/静态 Mesh 都使用原有 GPU sampler，增加随机向内深度。粒子以 Mesh 中心为基准向外散开，缩小、淡出。

当前 Q 中：约 0.86 s 完成亮扫，0.975 s 开始转换，1.49 s 表面清空，1.63 s 停止生成。粒子寿命为总时长的 0.24–0.36，最迟约 2.67 s 消退，早于 2.866667 s 的目标销毁事件，避免突然切断尾迹。

控制器保持 `Dissolve(Target, OriginWS)`、材质恢复、可见性恢复与资源清理。Q 的目标搜索、转向、抓取、动画、镜头和销毁时机均未改动。

## 既有验证与复核要点

- Blueprint 编译通过，两个 Niagara System 均为 UpToDate、无错误/警告。
- 实际 Q 的六个采样阶段均 `OutlineAmount = 0`；粒子阶段表面遮罩为 0；散开后生成率为 0。
- 在转换中取消：敌人可见，两个材质槽恢复为原对象，特效 Actor 数量为 0，移动解锁。
- 随后再次执行完整 Q：目标销毁、特效 Actor 数量为 0、移动解锁；使用原有处决镜头验收。

原有通用目标限制仍适用：未新增 ISM/HISM、Geometry Collection、任意主材质自动注入支持。本轮验收对象是当前关卡里的 Q 抓取敌人，不代表所有任意拓扑都具有精确内部填充。

以上为效果交付时的验证摘要；一次性脚本、原始记录、预览截图和外部备份已清理。当前版本以 UE 资产为准，本次文件清理未重跑 PIE。复核时在测试地图执行完整 Q、转换中取消、再次执行，检查目标材质/可见性恢复、特效清理和控制解锁。
