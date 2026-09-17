# 红莲圣天八极式 — 当前施工 Brief / V023

## 当前状态
- V023 阶段评审候选已保存；未批准最终 Primary Form Lock，非 UE-ready。
- 源文件：ArtSource/Characters/GurenSeitenHakkyoShiki/GurenSeitenHakkyoShiki_ProductionRigModelLock_v023.blend。
- V022 不覆盖；启动未保存现场已保留为 Checkpoints/V023_00_UserLiveState.blend。
- 用户最新要求高于施工单：参考图相似度优先，允许合理三维差异，不自造相似度分数。
- 历史说明归档：Archive/V023_PreviousInformation/AssetBrief_through_v022.md；不是当前指令。
- 当前入口：资产目录 `README.md`、`V023_REPORT.md`、`V023_REFERENCE_USAGE.md`、`V023_POSE_CONSTRAINT_AUDIT.md`。
- 最新回退：`Checkpoints/V023_12_ReviewCandidate.blend`；剩余胸甲曲线、肩腿块面和抓爪接触先收敛，再决定精雕。

## 身份与 Source of Truth
《Code Geass R2》最终形态红莲圣天八极式，约 4.5 m 高的高速近战飞行机甲。Blender 为外观源。项目批准其原作风格例外，不套 Q 版工业机器比例。
References/REFERENCE_INDEX.md 为本地参考入口；官网/动画决定身份，商品补三维连接，游戏补动作。来源不明不冒称原作设定。
保留小头、狭长面罩、后掠侧冠、橙金胸/后舱、红色甲壳、青绿传感器、深灰内构、细腰长腿、银色右臂、红核五金爪与粉色 Energy Wing。

## 当前范围
1. 修正胸—腹—腰：短中央胸尖、有厚度的下胸甲、内收腹甲和紧凑腰部。消除黑色梯形/杆状腰，不拉长橙金盾牌，不做腹肌。
2. 审核修复作者 IK、目标空间、肘膝平面、掌心方向和烘焙独立性；复用 RIG_AnimationAuthoring_V020。
3. 小范围完善头脸、左掌指根/拇指、右掌座/爪根、侧背连接，不堆微细节。
4. 飞行、单手抓头悬空、发射后从容降临是动作重点。
5. 当前入口精简为最新版；旧源文件、动作和几何备份可回退归档。

## 保护与技术约定
- SK_GurenSeitenHakkyoShiki 的 49 骨名称、层级、Rest、骨长、局部轴与 Root 默认锁定。
- 原生产 Object 命名、绑定、Parent/Pivot、材质槽身份、UV、Vertex Group 前后审计。
- 保留 7 Socket Candidate、右腕发射/重接链、五指独立及 Backpack/Wing Root。
- Energy Wing 4+4 主网格/翼展/红边与粉色主形锁定；仅完善翼根支撑及姿态。
- 历史动作保存在基线/归档；5 个 legacy Pose 不毁弃。允许整改本轮当前动作。
- 刚性件不得拉伸；不用隐藏、假连接或随意改生产骨来掩盖问题。
- 30 FPS，in-place Root；作者控制 → 求值烘焙 → 49 骨生产 Action。生产播放独立于作者层。
- 使用美术生产、blender-animation、blender-animation-rigging 三份 Skill。
- Reference → Blocking → Breakdown → Spline → Polish；本轮可评审 Blocking + 初步 Breakdown，不宣称最终动画。

## 当前动作契约
- Anim_CombatIdle：有重量的小幅悬停，掌心斜内/下。
- Anim_Flight：独立普通飞行，适度前倾、腿后拖、收臂、掌心内/后。
- Anim_EnergyWingBoost：压缩、扫翼、强前倾、流线峰值、释放，不拍翅。
- Anim_Descent：翼制动、收稳、标志降临；右掌向大腿内侧，拇指向前。
- Anim_Skill01_GrabDissolve：仅右手抓头、抬起整个人形目标、双脚离地、辐射停顿、空手恢复；左手平衡，溶解特效留 UE。
- Anim_ArmLaunch：瞄准、蓄力、分离、远端停留、回收、准确重接；肩/上臂连续。
- Anim_Ultimate_ArmLaunchDescentExplosion：发射后本体脱离、从容降临、英雄停顿与 ULTIMATE_EXPLOSION 标记；不做 Blender 假爆炸或长连打。
- 普通攻击与复杂连段暂缓。

## 验收与交付
正/侧/后/3⁄4、头胸、掌部与参考原图并排比较；关键姿势和实际连续播放都检查。重点是身份、胸腰衔接、脸可见、肘膝不反折、分离明确、抓头可信、飞行/降临有区别。
交付 V023、阶段回退点、当前报告/参考使用表/约束审计、精简模型/动作审查图和连续预览。只报告实际完成和验证内容；明显缺陷不批准进入下一阶段。
本轮不做正式 UV、贴图、微雕、UE 导出/导入、游戏代码或长过场。
