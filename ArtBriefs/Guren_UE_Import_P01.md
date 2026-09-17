# 红莲 UE 正式导入第一步

2026-09-10。授权：用户要求将 Blender 红莲的骨骼模型、材质和最新攻击动画正式导入 UE，创建直接继承 AGurenCharacter 的蓝图。

源：ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R8_NormalAttackCombo_ArmRoll.blend（当前已保存文件）。沿用批准的红/金/银/青/绿/粉身份色和造型。无需重新设计或重新制作动作。

输出目录：/Game/MineLearning/Characters/Guren。骨骼模型 SK_Guren、原有 59 骨层级（包含功能 socket 骨）、材质、AN_Guren_NormalAttackCombo 和 BP_GurenCharacter。

最终落盘名称：Skeletal/SK_Guren、Skeletal/SK_Guren_Skeleton、Animations/AN_Guren_NormalAttackCombo_R8、Blueprints/BP_GurenCharacter。导出副本显式转换厘米坐标、scale_length=0.01，避免 UE 丢失 Armature 单位缩放。首次单位排查资产可逆归档到 ImportDiagnostics，正式蓝图不引用。

仅在独立 UE 导出场景转换：204 个刚性部件转成每点单骨 100% 权重的蒙皮，合并为一个含独立几何岛的输出 Mesh，不焊接部件。保留评估后的倒角与法线，保留镜片 UV；无纹理部件补充基础 UV0 供合法导入，不更改源 UV。姿态逐帧采样 60fps F1–157，当前三次命中 F27/72/124。只输出本轮攻击动作，不输出历史测试 Action 或 IK 控制器。现有 socket 骨为运行时功能接口，明确保留。

导出以米制真实尺寸转换 UE 厘米；Blender 原前向 -Y 转为交付 +X，Z 向上，变换在导出副本记录。保持刚性部件、手掌发射/接回层级、翼根及各指骨；保护源 Rest、Action、绑定、模型、材质及用户手调状态。

材质按源 Principled 数值迁移，镜片纹理和 ColorRamp 保留可追踪来源，能量翼透明度和发光另行实现。源无贴图的材质不凭空增加贴图。骨架根无位移驱动的攻击使用原地动画，暂不接伤害判定或完整战斗系统。

蓝图装配模型和预览攻击动画，继承现有相机/飞行逻辑；配置必要输入引用，避免父类空输入资产。只做导入与角色装配，不更换游戏模式或修改用户关卡。

验收：源保护摘要、FBX 清洁重导入的骨架/蒙皮/姿态对照、UE 骨骼和材质槽/尺寸/动作时长检查、蓝图父类与编译检查、引擎图像或动画预览。只保存本轮资产。记录工具无法完成的验证，不以导入成功代替完整玩法验收。
