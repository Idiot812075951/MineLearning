# 当前任务：铁矿模块化资产第一轮

请先阅读项目 `AGENTS.md`、现有美术总 Skill，以及 `mining-ore-gameplay-asset/SKILL.md`。

本轮只操作当前铁矿石独立 Blender 源文件，不操作 UE、不导出、不提交 Git，也不得覆盖 OREBUDDY-07。

## 任务目标

将当前单体铁矿原型改造为一套可服务玩法的模块化铁矿资产：

- 1 个基础石体；
- 4 个可独立脱落的表层铁矿块；
- 2 个独立掉落碎块；
- 1 套组合预览。

## 一、基础石体

创建或重做：

`SM_Ore_Iron_Base_A`

要求：

- 保留 Q 版、明亮、可爱方向；
- 主体是不对称多面岩块，不是蛋、球、规则石墩；
- 具有 6～12 个清晰大块面和少量崩角；
- 尺寸约为中型矿点，维持当前 180cm 级别即可；
- 岩石材质为暖灰褐或暗红褐，非金属、高粗糙度；
- 不把可脱落铁矿块焊死在主体中。

## 二、表层铁矿块

创建：

```text
SM_Ore_Iron_SurfaceChunk_A_01
SM_Ore_Iron_SurfaceChunk_A_02
SM_Ore_Iron_SurfaceChunk_A_03
SM_Ore_Iron_SurfaceChunk_A_04
```

要求：

- 四个对象必须独立；
- 至少包含块状结晶和粗矿脉两种形态；
- 大小、方向和轮廓有明显差异；
- 约 25%～45% 体积嵌入 Base Rock；
- 视觉上像从岩石内部暴露，而不是贴纸、按钮或小黑虫；
- 至少一块作为主要视觉焦点，尺寸明显大于当前黑色小疙瘩；
- 使用深铁灰、略带暗红的金属材质；
- Metallic 约 0.75～1.0，Roughness 约 0.3～0.55；
- Origin 位于各自近似质心，便于后续脱落物理。

## 三、掉落碎块

创建：

```text
SM_Ore_Iron_DropChunk_A_01
SM_Ore_Iron_DropChunk_A_02
```

要求：

- 是独立对象；
- 与表层铁矿块属于同一视觉家族；
- 尺寸更小、轮廓更清楚，适合地面掉落和拾取；
- 不能是球形；
- Origin 位于近似质心。

## 四、预览与集合

按 Skill 建立或整理：

```text
COL_Ore_Iron_A
├─ GEO_EXPORT
├─ LAYOUT_PREVIEW
└─ PREVIEW_ONLY
```

保留现有 Camera、Key、Fill、Rim 和 Ground，但放入 `PREVIEW_ONLY`。

在 `LAYOUT_PREVIEW` 中展示一个组合矿点。所有 Surface Chunk 仍需保持独立，不允许合并。

## 五、预算

建议：

- Base Rock：800～2500 三角面；
- 单个 Surface Chunk：150～600 三角面；
- 单个 Drop Chunk：100～400 三角面；
- 完整组合不超过约 7000 三角面。

不要使用高细分去制造光滑。

## 六、完成后停止

完成后不要继续导出或操作 UE，只报告：

1. 保存路径；
2. 对象列表；
3. 每个对象的三角面数和尺寸；
4. 材质列表；
5. 哪四个对象可脱落；
6. 哪两个对象用于掉落；
7. Pivot、Transform、法线检查结果；
8. 一张组合预览图；
9. 一张所有模块分开展示图；
10. 当前仍然存在的美术问题。

不要写长篇总结，不要自行扩展任务。
