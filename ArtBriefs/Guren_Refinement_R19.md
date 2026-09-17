# 红莲 R19：结构精简与 Q 朝向修复

用户本轮明确授权：针对截图标记的大腿、小腿、背包、翼根叠层，移除不必要的装饰/重复盖板；处理 UE 可见的头部和右手指节切面；修复从敌人身后释放 Q 时被强行反向、倒退抓取的错误；清理 R18 补光的多方向光优先级警告。

源文件为当前实时 `V027A_R18_SurfaceFlow.blend`，先保存含未保存状态的备份，再在 `V027A_R19_Refinement.blend` 工作。保留红莲身份、头部轮廓和五指/爪尖、胸部镜片、能量翼、原比例和已改曲面。局部头盔/指节允许曲面与倒角修改。冗余几何移入隐藏的归档集合，排除出正式模型和导出；不永久删除回退数据。

保护原骨架、Rest Pose、Action、约束、Driver、材质槽与未修改件的 UV。保留承重关节、翼根铰链、抓取挂点和手指骨骼；不因视觉精简改动动画。UE 使用 Blender 正式导出更新；灯光只修正主光优先级。

Q 仅修正站位与朝向的空间参考系，保留原 Root Motion/Motion Warping、Montage、Notify 与技能生命周期。通过实际 PIE 测试敌人前/后/两侧、敌人旋转、初始面朝/背朝敌人、近距离及取消恢复。完成 C++ 可执行编译验证，检查导入后绑定与视觉。

## 本轮交付状态

当前静态外观 Source of Truth：`ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R19_Refinement.blend`。
UE 正式角色：`/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren`；关卡：`/Game/MineLearning/Maps/L_Guren_Retarget_Test`。

8 件冗余/穿插外甲移入 `R19_RemovedDetails_NON_EXPORT`，正式网格从 204 件变为 196 件；16 件修改控制网格，手掌另修复倒角、微小退化边和法线。完成源文件保护检查、17 个修改件的网格检查、UE 导入与近景复核。C++ 完整编译成功，重启后 8 组实际 Q 测试通过。明细见 `Docs/Guren_Refinement_R19_Delivery.md`。
