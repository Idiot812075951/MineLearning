# 红莲圣天八极式 — 当前交付入口 R19

当前静态外观与交付范围见 [R19 Asset Brief](Guren_Refinement_R19.md) 和 [R19 交付说明](../Docs/Guren_Refinement_R19_Delivery.md)。Blender Source of Truth 为 `ArtSource/Characters/GurenSeitenHakkyoShiki/V027A_R19_Refinement.blend`；UE 正式网格为 `/Game/MineLearning/Characters/Guren/Skeletal/SK_Guren`，验证关卡为 `/Game/MineLearning/Maps/L_Guren_Retarget_Test`。

Q 技能的当前配置见 [配置说明](../Docs/Guren_Q_Configuration.md)，辐射溶解外观见 [Final Scatter](RadiantDissolve_FinalScatter.md)。后续工作以这些当前说明及用户本轮要求为准。

## V024 历史施工记录

以下内容保留用于追溯 V024 阶段，不再是当前施工入口。

V024 阶段范围见 [V024 Asset Brief](GurenSeitenHakkyoShiki_V024.md)。上一版 Brief 保留在 [V023](GurenSeitenHakkyoShiki_V023.md)。

源文件：`ArtSource/Characters/GurenSeitenHakkyoShiki/GurenSeitenHakkyoShiki_TargetedFormHandsIK_v024.blend`。
交付说明：`ArtSource/Characters/GurenSeitenHakkyoShiki/V024_REPORT.md`。

当前状态：V024 定向修复评审版本。改善胸腹腰胯、头脸、左手、Skill01 一拇四指包覆和作者 IK 入口；不是最终模型锁定或 UE-ready。

R2 官方/动画决定比例轮廓，商品近景补三维连接；参考图库仍在资产目录 `References/REFERENCE_INDEX.md`。保持红甲、橙金胸/座舱、青绿镜片、深灰内构、银色辐射臂、红核金爪和粉色推进翼。不自造相似度分数。

保留 V023、用户 IK 试验现场和阶段回退点。本轮未修改生产/作者 Rest、骨长、骨名、主层级、原绑定及挂点；仅 Skill01 定向重烘焙，其他六动作保留。脚本使用原作者骨架和现有操作入口，不新增替代 Rig。

后续不得擅自扩大到新攻击包、翼型重做、正式 UV、UE 导出/导入或独立溶解实验。工作遵循三维资产生产、动画、动画绑定三份 Skills，并以可见参考和实际模型/动作比较为准。
