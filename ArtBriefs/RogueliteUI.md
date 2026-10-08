# 肉鸽 UI 当前资产说明


## 2026-10-03 内容扩充图标

本轮增加 11 张独立图标，沿用深蓝背景、钢铁/金色主体与青色边光。Source of Truth 为 `ArtSource/UI/RogueliteExpansion/<ID>.png`；对应 UE 的 `UI/Art/T_Expansion_<ID>` 与 `MI_Icon_<ID>`。不替换旧源图，不保留生成过程脚本或验收帧。全部通过内置 imagegen 独立生成，未使用 API/CLI 降级。

最终提示模板（每个 ID 替换 Subject）：

> Create one square premium sci-fi roguelite buff card icon. Subject: {Subject}. Single very clear large central silhouette, polished stylized game illustration, engraved steel with gold trim, cyan rim light, deep navy almost black background, high contrast readable at 64 pixels, restrained particles, full object inside generous margin. No text, no letters, no numbers, no borders, no collage or grid. This is a standalone UI texture for a mining robot game.

| ID | Subject |
| --- | --- |
| LongDrill | a single very long steel spiral mining drill bit pointing upper right with cyan reach streak |
| HeavyDrill | a single thick heavy steel mining auger with huge tungsten teeth, orange impact chips |
| ChargedDrill | a steel mining auger wrapped in bright cyan electricity and one glowing power cell |
| GiantSlayerCarrier | a small agile humanoid mining robot with inward shrinking arrows and cyan motion streaks |
| GoliathCarrier | a massive broad shouldered mining robot carrying a large crate, outward scale arrows |
| RelayBaton | one robotic hand passing an orange ore chunk to another robotic hand, cyan forward motion |
| CoopHeavyCarry | two small steel worker robots visibly holding opposite handles of one large blue sci-fi freight crate |
| LongBarrel | one futuristic rifle with an unmistakably very long slender barrel and a distant crosshair |
| ShortBarrelAssault | one short barrel compact assault rifle with a blazing orange muzzle and several speed streaks |
| Overclock | a glowing orange computer processor surrounded by electric lightning and accelerated cyan arrows |
| MainThread | one bright gold central processor connected by cyan lines to three smaller blue robot worker heads |

已检查全部源图主体与语义，并在 UE 中导入；材质使用完整 UV，运行时纹理上限 512。卡牌、效果、永久节点和召唤师均引用对应材质。

日期：2026-10-03。范围为肉鸽 UI 与运行时战斗反馈，不修改角色基础外观、场景、Rig、Animation 或其他美术源文件。

## 目标与实际资产

玩家能通过出生点 Cube 商店、HUD 天赋/图鉴入口和短卡牌文案完成付费升级。矿区科技主题，深蓝背景、钛金属对象、青色能量与少量金色。卡面优先大图标、名称、效果数值和可用状态；完整规则留在 Tooltip。

当前源文件为 ArtSource/UI/Roguelite/RogueliteIconAtlas.png，以及 Buff_Attack.png、Buff_Move.png、Buff_Cast.png、Buff_Collector.png、Buff_Conqueror.png、Buff_Roamer.png。UE 通过 UI/Art/T_RogueliteIconAtlas、T_Buff_*、M_RogueliteIcon 和十二个 MI_Icon_* 使用，位于 /Game/MineLearning/GameplayRuntime/。旧图集仍服务六种图标，其余三格不再引用。DT 的 Icon 可替换为其他 UI 材质。图像无文字，玩家可见文本均在 UMG/DT 中。

新卡语义：致命征服者采用金色斧刃、节奏双箭头与青色流光；漫游枪手采用疾跑靴与正在发射的手枪，青色轨迹表达移动射击。分类/单位角标覆盖在图标顶部，减少独占行高。新增技能栏模式文字为 UMG 独立控件，由武器状态事件刷新。

### Buff_Conqueror.png

2026-10-03 替换为 AI 融合图。参考 Riot 官方 Data Dragon 的 [征服者](https://ddragon.leagueoflegends.com/cdn/img/perk-images/Styles/Precision/Conqueror/Conqueror.png) 与 [旧版致命节奏](https://ddragon.leagueoflegends.com/cdn/img/perk-images/Styles/Precision/LethalTempo/LethalTempoTemp.png)。两张参考保存在本目录对应 ArtSource/UI/Roguelite/References 中，用于造型来源追溯，非验收截图。正式纹理 T_Buff_ConquerorFusion，由原 MI_Icon_Conqueror 引用。使用 imagegen 对这两张图片融合，保持方形深蓝底，无文字；金斧和双节奏箭头在小尺寸仍能识别。

本次融合提示：

> Create one production square buff icon named conceptually Lethal Conqueror, no lettering. Fuse the two provided classic League of Legends rune references into a NEW cohesive polished dimensional icon for a mining sci-fi roguelite. Strong single golden battle axe silhouette from first reference, vertical handle and left sweeping axe blade. Combine with the second reference's two sharp golden tempo chevrons flanking the handle and a restrained curved cyan energy trail. Warm gold dominates weapon, tiny titanium inlays and cyan edge light, dark midnight navy background. Large iconic object, legible at 40 pixels, center fills 80%, clean distinct silhouette, few details, premium game inventory artwork. No text, no logos, no UI border, no collage, no separate icons. Square opaque background. Preserve recognizable axe and tempo-arrow visual meaning, not previous sword/crown concept.

### Buff_Roamer.png

> Use case: stylized-concept. Production square game buff icon for a mining sci-fi roguelite, single large readable object composition, brushed titanium, cyan energy and warm gold accents, midnight navy radial backdrop. Clean polished dimensional game item art, readable at 48 pixels, generous safe margin. No text, digits, borders, watermarks or logos. One futuristic running boot with small heel wing, paired with a compact titanium pistol above it firing a golden round to the right; three cyan horizontal motion trails behind both. Unmistakable moving and shooting, no human figure, no multiple weapons, clean simple composition.

当前语义：攻击速度（刀刃连斩残影）、移动速度（疾跑靴和速度轨迹）、施法速度（施法手势和沙漏）、搜集狂热（手套抓取矿石），其余沿用力量钢拳、三颗子弹、能量弹、双弹夹、黄金 AK、复制人幻影。普通矿工暂复用力量图标。卡片顶部增加可配置 UnitLabel，Gunner 专属卡明确写 Gunner，不向图像烘焙单位文字。

## 生成来源

本轮通过 imagegen 生成一张实际九格图集，随后通过 Unreal MCP 导入。没有下载在线图标，没有离线修改 UE 资产，没有用图像后处理脚本裁剪或重绘。生成提示如下：

> Create a production-quality 3x3 square game UI icon ATLAS for a mining sci-fi roguelite. Nine perfectly equal square tiles in an exact 3 columns by 3 rows grid, no gutters, no outside margins, no labels, no lettering, no numbers, no watermark. Every object centered within its tile with generous 20 percent safety margin. Each tile a dark midnight-navy subtle radial vignette backdrop, glossy brushed titanium objects, crisp readable silhouettes, restrained luminous cyan accents and gold accents, dimensional premium game item art, consistent camera/light and scale. Top row left: a titanium rifle bolt with two cyan speed streaks representing attack speed; top middle: a luminous cyan spell crystal with circular cast energy arcs; top right: a heavy armored steel fist glowing warm amber representing strength. Middle row left: a magnet pulling three glowing ore crystals with cyan sparks, collector frenzy; middle middle: exactly three brass rifle rounds standing together with cyan motion trails, triple burst; middle right: one powerful luminous gold bullet surrounded by two concentric cyan shock rings, super bullet. Bottom row left: a tactical rifle magazine splitting into two translucent cyan doubled magazines, super magazine; bottom middle: a clearly recognizable golden AK style assault rifle diagonally posed, golden gun; bottom right: a cyan translucent holographic human silhouette with a faint second silhouette behind, phantom clone. All nine tiles fill same square size, edges aligned on thirds, strictly no overlapping across tiles. The image is an actual icon atlas, not a UI screenshot or menu. High-quality clean rendering, sharp at small thumbnail scale.

## 验收与维护

UMG 在真实 PIE 中检查准备、天赋、购买、三选一、图鉴上下分组和中控。观察过小卡图标的辨识度、DPI 缩放、按键入口和 HUD 避让；自动化按钮使用实际控件委托，不模拟桌面鼠标键盘。验收截图属于 Saved 临时输出，不作为正式美术交付。

三选一与图鉴复用 WBP_UpgradeCard，节点使用 WBP_TalentNode/Link，主菜单在 WBP_RogueliteHub。视觉的 Source of Truth 是这些正式 UMG 资产与上述实际使用的源图片；后续修改沿用同一功能，不留阶段图、生成脚本或备用图集。

## 辨识度修订与参考

参考 [Dota 物品图标对照](https://liquipedia.net/dota2/DotA_item_comparison)、[速度之靴图标演变](https://liquipedia.net/commons/File%3ABoots_of_Speed_item_icon_progress.png) 与 [Guild Wars 2 增益图标](https://wiki.guildwars2.com/wiki/Boon)。设计取舍是先用可识别的物体和动作表达含义，再用速度轨迹补充“加速”；不依靠玩家理解抽象晶体或机械零件。未复制或下载这些游戏的图标。四张新图通过 imagegen 独立生成，没有图像脚本裁剪或重绘。

以下为最终生成提示，分别对应同名 PNG：

### Buff_Attack.png

> Use case: stylized-concept. Asset type: production game buff icon, one square icon, polished dimensional illustration for a mining sci-fi roguelite. Dark midnight navy plain radial background, brushed titanium, vivid readable cyan accents, restrained warm highlights. Large central silhouette fills 75% of frame with generous safe margins. Very legible at 48 pixels, clean strong edges, simple composition, no ornamental frame, no letters, numbers, logos, or watermark. Meaning: FASTER ATTACKS. A single sharp short titanium sword slashing diagonally, with two separated translucent blade afterimages following it and three crisp cyan curved motion streaks. Emphasize rapid repeated swings, not heavy strength. One solid sword only, afterimages subordinate. No gun parts, no fists, no clock.

### Buff_Move.png

> Use case: stylized-concept. Asset type: production game buff icon, one square icon, polished dimensional illustration for a mining sci-fi roguelite. Dark midnight navy plain radial background, brushed titanium, vivid readable cyan accents, restrained warm highlights. Large central silhouette fills 75% of frame with generous safe margins. Very legible at 48 pixels, clean strong edges, simple composition, no ornamental frame, no letters, numbers, logos, or watermark. Meaning: FASTER MOVEMENT. One futuristic athletic running boot angled forward in a running leap, a small wing on its heel and three bright cyan horizontal speed trails behind the heel. Unmistakably a shoe, large clean silhouette. No weapons, no magical crystals.

### Buff_Cast.png

> Use case: stylized-concept. Asset type: production game buff icon, one square icon, polished dimensional illustration for a mining sci-fi roguelite. Dark midnight navy plain radial background, brushed titanium, vivid readable cyan accents, restrained warm highlights. Large central silhouette fills 75% of frame with generous safe margins. Very legible at 48 pixels, clean strong edges, simple composition, no ornamental frame, no letters, numbers, logos, or watermark. Meaning: FASTER SPELL CASTING. A single open gloved hand casting a bright small magical cyan star over the fingertips, paired with one simple clearly readable glowing hourglass to the side. Hand and spell dominant, hourglass secondary. Crisp luminous casting arc, no closed fist, no loose crystal item, no weapon.

### Buff_Collector.png

> Use case: stylized-concept. Asset type: production game buff icon, one square icon, polished dimensional illustration for a mining sci-fi roguelite. Dark midnight navy plain radial background, brushed titanium, vivid readable cyan accents, restrained warm highlights. Large central silhouette fills 75% of frame with generous safe margins. Very legible at 48 pixels, clean strong edges, simple composition, no ornamental frame, no letters, numbers, logos, or watermark. Meaning: RAPID RESOURCE PICKUP / COLLECTOR FRENZY. One open titanium work glove cupping and scooping three chunky iron ore pieces into the palm, the wrist trailing two clean cyan speed streaks. Ore and collecting gesture unmistakable, simple composition. Warm copper ore facets and cyan energy. No magnets, no machinery coils, no weapons, no fist.


## 运行时战斗反馈（本轮小型新增）

Source of Truth：正式 UE 材质 `GameplayRuntime/FX/M_AttackRange`、`M_StackAura` 与组件配置。红色代表暂时狂热和距离警示，属于已明确要求的玩法反馈，不改变角色原有黄色工业外壳或黄金 AK 身份。

圆圈使用无碰撞平面和透明红色环材质，半径由实时战斗射程驱动；显示两秒后隐藏。狂热使用附加 Overlay 的 Fresnel 红橙轮廓与时间脉动，Intensity 随层数变化；不改原模型、槽位材质、骨骼和动画。没有新增 Blender 文件或离线 UE 资产脚本。状态行图标 44×44 设计单位，正文 18，角色技能键提示独立于图标。
