# 红莲 Q 技能测试交付 — 2026-09-13

## 最新调整：长距离冲刺与镜头冲击

本节取代下方首版的 500 cm 距离限制：最大选取范围为 **2000 cm**，200 cm 内仍直接抓取。按玩家要求采用游戏身位尺度，不按现实码数换算。1000 cm 目标突进约 0.76 秒，2000 cm 约 1.05 秒，继续由 Root Motion + Motion Warping 驱动。

- 冲刺：FOV 适当增加 4 度，速度感更强；粉红姿态残影每约 0.065 秒产生一次，0.24 秒淡出，最多同时 4 个，无碰撞、无投影。
- 抓头接触：0.22 秒真实时间的 0.25 倍慢镜头，开始用 0.7 秒平滑绕到红莲正面偏侧；接触时不震屏。
- 抓取和提起：视角平滑拉近，举起辐射阶段 FOV 从默认 90 收到 80；不覆盖滚轮设置。
- 溶解开始：触发一次 0.35 秒震屏。溶解完成不再额外震动，镜头用 0.45 秒恢复原视角；取消、结束或离场均清理残影和临时时间倍率。
- `BPC_GurenQPresentation` 可调 `ImpactStrength`、`CloseUpFOV`、`ContactTimeScale` 和 `AfterimageMaterial`；QSkill 的 `SelectionRange` 可调最大距离。
- 新材质：`/Game/MineLearning/Characters/Guren/VFX/M_QDashAfterimage`。
- 已验证 1000/2000 cm 完整流程（终点误差约 1.6/2.4 cm）、2001 cm 拒绝、近距离慢镜头中取消。最终 FOV=90、时间倍率=1、残影数量=0、移动解锁。记录：`Saved/GurenQ/qa_impact.json`。
- 保留同一地图，出生点附近目标改为 1000 cm；另设长距离站点和近距离抓取目标，重新进入 PIE 可恢复目标。

完整配置（镜头、震屏、慢镜头、距离、抓取点、Montage/Notify、溶解、残影及代码固定值）：`Docs/Guren_Q_Configuration.md`。镜头验证记录：`Saved/GurenQ/qa_execution_camera.json`。

## 首版交付记录（以下 500 cm 数值已由上方替换）

已在现有 `/Game/MineLearning/Maps/L_Guren_Retarget_Test` 接通 Q：500 cm 内选最近测试目标；200 cm 内直接抓取，200–500 cm 使用 Root Motion + Motion Warping 突进，然后右手抓头、提起、约 2.87 秒辐射溶解、删除目标并恢复控制。复用已有 `BP_RadiantDissolve_Test`，未另做溶解材质。

## 来源与资产

- Blender 来源：`ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R16_QGrabHeadIK.blend` 的当前用户状态，导出前副本在 `UEExport/QSkill/R16_UserLive_BeforeExport.blend`。原骨骼层级、Rest Pose 和源 Action 保留。
- 动画：`/Game/MineLearning/Characters/Guren/Animations/Q/AN_Guren_Q_Dash`、`AN_Guren_Q_GrabHeadIK`。
- Montage：同目录 `AM_Q_Dash`、`AM_Q_GrabDissolve`；QFullBody Slot 和 Q Events 通知。
- 蓝图：`/Game/MineLearning/Characters/Guren/Blueprints/BPC_GurenQPresentation`、`BP_QGrabTestDummy`；修改现有 `BP_GurenRetargetTest` 和 `ABP_GurenLocomotion` 接入。
- Skeleton：既有 `SK_Guren_Skeleton` 新增 `Q_GrabHead` Socket，附于 `radiant_hand_r`，未改骨骼结构。
- 测试地图：新增三个小白人，归入 `Q Skill Targets` 文件夹。

## 代码

`Source/MineLearning/Manifestation/Guren/`：新增 `GurenQSkillComponent`（流程）、`GurenQPresentationComponent`（动画与效果）、`QGrabTestDummy`（测试目标）、`GurenQAssetSetup`（编辑器 Socket/Slot 配置桥接）。修改 `AGurenCharacter.h/.cpp` 接入 Q 输入和互斥。`MineLearning.uproject` 与 `Source/MineLearning/MineLearning.Build.cs` 启用 MotionWarping。

## 验证

- C++ 完整编译成功。
- PIE 验证 165 cm 直接抓取、350 cm 完整突进流程、480 cm 突进后辐射阶段中断、无目标、飞行中拒绝 Q、重复 Q、攻击/飞行互斥、目标销毁。
- 5 FPS 完整流程完成；修复 Notify 早于当前帧 Root Motion 应用而误判突进终点的问题。
- 辐射中断后解除绑定、恢复移动，并恢复原材质对象，避免残留或嵌套动态材质。
- 实际 Slate Q 键输入成功触发 Dash → Grab；完整结束日志记录在 `Saved/Logs/MineLearning.log`。
- 已在 PIE 中观察右手举起敌人、头部朝向敌人、辐射溶解的表现。
- 详细检查记录：`Saved/GurenQ/qa_results.json`、`qa_materials.json`。后者为最终材质修复后的专项复查，前者保留较早整套测试结果。

## 操作

1. 打开 `L_Guren_Retarget_Test`，点击 Play。
2. 点击游戏视口获取键盘输入，按 Q；出生点附近目标可直接测试突进全流程。
3. 靠近其他目标到 200 cm 内再按 Q，检查直接抓取。
4. 远离目标超过 500 cm 按 Q，不应释放。
5. 重启 PIE 恢复被溶解的三个目标。

无需手动连接资产。当前为单机小白人 Demo，不含正式伤害数值、任意敌人适配或联网；抓取站位和头部参考按当前测试小白人尺寸校准。静态模型外观沿用用户已确认的红莲，本轮验收限于动画导入与技能流程。
