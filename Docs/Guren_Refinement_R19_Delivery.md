# 红莲 R19：结构精简、曲面与 Q 朝向修复

本轮结果已进入正式 UE 角色和 `L_Guren_Retarget_Test` 关卡。源文件为 `ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R19_Refinement.blend`。原 R18 与本轮开始时含未保存状态的 `Refinement_R19/00_UserLive_R18.blend` 保留。

## 模型改动与原因

| 问题 | 处理 |
| --- | --- |
| 大腿背面孤立的小凸起 | 射线定位发现来自 `Thigh_SideCarapace` 穿出主甲，并非主甲需要继续加细分；这对重叠外甲移入归档。 |
| 大腿、小腿、翼根堆叠装饰 | 归档左右 `Thigh_Fin`、`Calf_RearTrack`、`Wing_StrutFace`，保留主甲、翼根铰链和支撑件。 |
| 背包、小腿仍显方块 | 调整背包主体、小腿与机械外壳、胫甲回边的曲面过渡，保持原绑定和局部外包范围。 |
| 头盔三角碎面 | 定位到 `Head_Visor` 弯曲外壳的非平面四边面；重建连续曲面，同时调整后壳与顶冠。 |
| 银色指节切面 | 根据原截面的圆心、半径方向及锥度，把五个指节的圆柱壁重建为均匀的 48 边截面，增加真实端部倒角。保留金色爪尖、连接中心与骨骼。 |
| 手掌硬边 | 原修改器使用 WEIGHT，但网格没有倒角权重，因而未生效；改用按角度生效的倒角和面法线。焊接倒角产生的两个微小退化边，避免零面积面。 |

共归档 **8 件**，正式网格对象 **204 → 196**。归档位于 `R19_RemovedDetails_NON_EXPORT`，不进入 FBX，可恢复。控制网格改动 16 件，另修改手掌的倒角和法线，共检查 17 件。

本轮没有改变胸部镜片、能量翼或材质参数，保留 R18 的哑光层级。本轮属于视觉整理，局部曲面增加了细分：LOD0 为 **157,296 顶点、313,822 三角形**，原 R18 为 288,618 三角形。删除叠层件不等同于本轮总三角形减少。

## Q 朝向修复

原实现直接采用敌人 `GrabStandPoint` 的世界旋转和固定站位，角色从另一侧接近时仍被转到同一方向。现在按“角色 → 敌人”的水平向量求朝向，并把原本校准好的右手抓取偏移绕目标一起旋转。冲刺、站位和抓取使用同一空间方向。

只修改 `Source/MineLearning/Manifestation/Guren/GurenQSkillComponent.cpp` 的 `TryCast` 站位计算。原 Root Motion、Motion Warping、Montage、Notify、抓取挂点、取消恢复和阻挡检查保留。

开发版常规构建成功，随后重新打开 UE，验证正式二进制。重启后实际 PIE 的 8 组测试全部通过：正面初始背对、背后初始背对、背后正对、左右两侧、目标旋转 127°、近距离背后、1900 cm 背后完整处决。检查转身方向、冲刺移动方向、实际右手附着和移动解锁；本次重启后到位误差约 **0.2～1.8 cm**。

测试脚本为 `Tools/GurenQ/qa_approach_direction.py`，结果在 `Saved/GurenR19/q_direction_results.json`。为隔离朝向问题，测试暂时关闭 PIE 障碍碰撞，保留关卡地面；结束 PIE 后恢复原关卡。关闭后台限帧进行正式回归，随后恢复偏好。另一次 3 FPS 后台检查中，斜向接近出现 56.8 cm 残余误差，触发原有 55 cm 保护并取消；本轮未放宽该保护。

## UE 交付与检查

- 正式网格：`/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren`。
- 保留候选资产：`/Game/MineLearning/Characters/Guren/Refinement_R19/SK_Guren_R19`。
- 59 根骨骼、原 Skeleton / Physics Asset、19 个材质槽保留；最大导入 Rest 变换误差 0.000293 cm 以下。
- 导入源法线和切线，不让 UE 重新计算覆盖曲面结果。
- 源骨架、Action、对象父级、骨骼绑定、对象变换及 UV 比较通过；17 个修改件均无非流形边、零面积面和非有限坐标。
- 主光 `Sun` 的 Forward Shading Priority 为 1，补光为 0。灯光强度保持原值，重启后编辑器截图确认竞争警告消失。
- 实际关卡已查看头部、右手、背部和整机近景；另外实际执行背后冲刺与抓头并截图，确认朝向和新手部外形。

网格和关卡的发布前备份在 `Saved/GurenR19/BeforePublish`；构建日志在 `Saved/GurenR19/FullBuild.log`。

## 图像与复现材料

图像均在 `ArtSource/Characters/GurenSeitenHakkyoShiki/Refinement_R19/`：

- `05_Final_threequarter.png`：Blender 整机。
- `05_Final_legs.png`：去除穿出侧甲后的腿背面。
- `06_UE_Head.png`、`07_UE_Hand.png`、`08_UE_Back.png`、`09_UE_Hero.png`：UE 头部、手部、背部、整机。
- `10_UE_RearApproach.png`、`11_UE_RearGrip.png`：实际背后接近和抓取。

同目录保留 `r19.py`、`export_r19.py`、UE 导入/发布配方与 JSON 校验结果。建模配方从 R18 备份的新 R19 副本执行一次；不应在已处理好的 R19 上重复运行 `apply()` 或 `precise_finish()`。
