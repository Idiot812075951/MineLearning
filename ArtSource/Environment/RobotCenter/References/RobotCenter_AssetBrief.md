# Robot Center Blender 施工清单

## 0. 施工目标

在 Blender 中制作一套独立的 **Robot Center / 机器人中心** 场景资产。

它不是可进入室内的建筑，而是：

> **一个大型半开放工业平台 + 一个中央主体 + 左右两个独立功能模块。**

功能布局固定：

```text
                    ROBOT CENTER
              ┌─────────────────┐
              │   中央主机/横梁   │
              └───────┬─────────┘

        Robot Shop              Transform
        机器人售卖区             玩家幻化区
            □                       ○

────────────────────────────────────────
              大型工业平台
```

左边负责：

- 机器人售卖终端
- 机器人展示台

右边负责：

- 玩家幻化终端
- 幻化站立平台
- 扫描结构

中央负责：

- 统一 Robot Center 视觉主体
- 能源/控制主机
- 将左右两个模块在视觉上连接起来

---

# 1. 美术方向

必须符合 MineLearning 当前整体画风：

> **Q 版工业科幻，结构简单、轮廓清晰、细节少而准。**

颜色角色：

```text
工业暗黄
→ 工程/制造设施主要识别色

枪灰 / 深灰
→ 主骨架、底座、内部机械结构

银灰
→ 少量机械连接件、导轨、金属件

青色
→ 屏幕、功能灯、扫描灯、能源状态

橙色
→ 极少量 Warning / 危险提示
```

禁止做成：

- 白色未来实验室
- 大片玻璃
- 全身蓝色霓虹
- 写实重工业
- 满身螺丝和小面板
- 两个普通自动售货机摆在地上

原则：

> **远看先读轮廓，中距离读功能，近看才看到细节。**

---

# 2. Blender 文件

新建独立文件：

```text
RobotCenter.blend
```

不要继续放进 MiningArea 的 `.blend`。

建议 Collection：

```text
RobotCenter
├── 00_Reference
├── 01_Base
├── 02_MainStructure
├── 03_RobotShop
├── 04_TransformStation
└── 05_Detail
```

---

# 3. Reference

在 `00_Reference` 中放：

- Human 高度 Proxy
- OreBuddy 大小 Proxy
- 如有必要放仓库 / 处理机简单尺寸参考

这些对象：

- 不参与最终导出
- 只用于检查比例

如果无法直接导入角色模型：

使用简单 Cube / Capsule 代替即可。

不要为了 Reference 花时间。

---

# 4. 第一阶段：只做 Blockout

第一阶段禁止制作小细节。

只制作以下 7 个主要模块：

```text
1. Base_Platform
2. Main_Frame
3. Main_Core
4. Shop_Terminal
5. Shop_DisplayPad
6. Form_Terminal
7. Form_Pad
```

外加必要的：

```text
8. Access_Ramp / Access_Steps
```

完成 Blockout 后必须先检查整体轮廓，再继续。

---

# 5. Base Platform

创建大型整体工业底座。

命名：

```text
SM_RobotCenter_Base
```

### 造型

不要使用纯矩形。

推荐：

```text
俯视

       /────────────────────\
      │                      │
      │                      │
      │                      │
       \────────────────────/
```

使用：

- 切角矩形
- 八边形感
- 大块面

避免复杂外轮廓。

### 比例建议

整体视觉比例：

```text
宽 : 深 : 高
约
12 : 6 : 5
```

不是强制米数，只作为比例关系。

Robot Center 应：

> **宽明显大于高。**

---

# 6. 通行入口

正面至少制作一个主要通行入口。

本次必须修改概念图中的陡台阶设计。

目标：

> **更长、更缓、更容易让 Human 与机器人通行。**

推荐优先：

```text
宽缓坡道
```

或者：

```text
非常浅的 2~3 级长台阶
```

不要：

```text
短
陡
窄
```

### 推荐结构

```text
侧视

平台
──────────────
            /
          /
        /
──────
地面
```

坡道需要：

- 宽
- 长
- 缓
- 两侧少量工业护边

不要做真实建筑里很密的楼梯。

机器人中心属于工业设施，通道应偏向：

> **设备坡道 / 物流平台**

---

# 7. Main Frame

命名建议：

```text
SM_RobotCenter_MainFrame
```

核心结构：

```text
正视

         ┌────────────────┐
         │  ROBOT CENTER  │
         └────────────────┘
          │              │
          │              │
        左支柱          右支柱
```

制作：

- 左右两根粗支柱
- 顶部横梁
- 中央 Logo / 标识位置

不要做墙。

不要把后面完全封死。

目标：

> 让整个设施拥有“建筑级体量”，但依然是开放式设备。

### 造型语言

支柱：

- 粗
- 短
- 稳
- 稍微 Q 化

顶部横梁：

- 厚
- 横向延伸
- 不要太细

适当 Bevel。

---

# 8. Main Core

在后方中央增加一个核心主机：

```text
SM_RobotCenter_MainCore
```

大致：

```text
      Main Frame

         ███
       ┌─────┐
       │ Core│
       │  ◉  │
       └─────┘
```

它负责告诉玩家：

> 左右两个功能区都属于同一套 Robot Center 系统。

可以包含：

- 一块主屏幕
- 一个青色能源核心
- 1~2 个大型机械连接件

禁止：

- 密集按钮
- 十几个屏幕
- 大量散热孔

---

# 9. 左侧：Robot Shop

左侧造型语言：

> **方形、稳定、机械、制造。**

Collection：

```text
03_RobotShop
```

## 9.1 Shop Terminal

命名：

```text
SM_RobotCenter_ShopTerminal
```

结构：

```text
   ┌─────────┐
   │ Screen  │
   ├─────────┤
   │ Machine │
   └─────────┘
```

特点：

- 方形主体
- 正面屏幕
- 低矮操作台
- 黄色外壳 + 深灰骨架
- 少量青色灯

屏幕位置以后 UE 做材质即可。

不要在 Blender 做复杂 UI。

---

# 10. Robot Display Pad

命名：

```text
SM_RobotCenter_ShopDisplayPad
```

制作一个用于展示待购买机器人的底座。

形状：

> 方形 / 切角方形。

不要与幻化台一样做纯圆。

### 内容

平台：

- 中间平坦
- 周围一圈青色发光槽
- 边缘少量黄色结构
- 机器人可以站在上面

第一版不要做机械臂制造动画。

不要做完整机器人装配流水线。

以后展示机器人：

```text
OreBuddy
Gunner
Carrier
```

由 UE 动态 Spawn / 展示。

---

# 11. 右侧：Transform Station

右侧造型语言：

> **圆形、扫描、能量、重构。**

与左边必须一眼不同。

Collection：

```text
04_TransformStation
```

---

# 12. Form Pad

命名：

```text
SM_RobotCenter_FormPad
```

核心形状：

```text
俯视

      ╭───────╮
     │    ◉    │
      ╰───────╯
```

推荐圆形或近圆形。

玩家站在中央。

平台需要：

- 明确中心点
- 外圈机械环
- 青色灯槽
- 外侧 3~4 个较大的机械卡扣

不要堆几十个小零件。

---

# 13. Transform Scanner

命名：

```text
SM_RobotCenter_FormScanner
```

推荐做成：

```text
正视

        ╭────────╮
       /          \
      │            │
      │   Player   │
      │            │
       \          /
        ╰────────╯
```

可以使用：

- 两侧机械柱
- 上方半圆扫描架

不要做完整密封舱。

不要做玻璃舱。

不要做门。

玩家从正面直接走进去。

### 扫描灯

结构中预留：

- 左侧扫描灯
- 右侧扫描灯
- 顶部扫描灯

实际发光、扫描动画在 UE 内做。

Blender 只负责结构。

---

# 14. Form Terminal

旁边增加一个小型控制终端：

```text
SM_RobotCenter_FormTerminal
```

不要挡住玩家进入扫描台。

建议位于：

```text
扫描台右前方
```

或者：

```text
扫描台侧面
```

尺寸明显小于扫描结构。

---

# 15. 左右模块视觉区别

必须保证即使关闭所有 UI、文字、灯光：

玩家仍然能通过轮廓判断：

```text
左边
Robot Shop
= □ 方

右边
Transform
= ○ 圆
```

这是本资产最重要的视觉设计之一。

---

# 16. 中央与左右连接

从 Main Core 分别连接：

```text
Main Core
  ├── Shop
  └── Transform
```

建议只使用：

- 1 根粗管 / 电缆连接 Shop
- 1 根粗管 / 电缆连接 Transform

不要做管线森林。

可以用：

```text
Curve + Bevel
```

制作。

要求：

- 粗
- 易读
- 不挡通行
- 不穿过玩家主要路径

材质：

深灰 / 黑色橡胶。

连接头可以有少量青色光环。

---

# 17. 第二阶段：结构细化

只有 Blockout 通过后才允许进入。

增加：

- Bevel
- 大型面板层级
- 机械支撑
- 连接件
- 大型通风结构

控制原则：

每个大型模块最多增加：

```text
2~4 个主要二级细节
```

不要每块面都添加东西。

---

# 18. 第三阶段：功能细节

只做有意义的细节。

允许：

- 屏幕边框
- 警告条
- 灯条
- 能源接口
- 机械连接头
- 简单检修面板

禁止为了“丰富”添加：

- 无意义螺丝阵列
- 无意义几十个按钮
- 小管线
- 小齿轮
- 随机凹槽
- 大量文字

遵循：

> **细节必须解释功能，否则不要做。**

---

# 19. 材质槽

建议整套 Robot Center 共用少量材质。

例如：

```text
M_RC_Yellow
M_RC_Gunmetal
M_RC_Silver
M_RC_DarkRubber
M_RC_Cyan
M_RC_Orange
```

尽量复用项目已有材质体系。

不要一个零件一个材质。

不要为了 Blender 漂亮单独制作复杂贴图。

当前项目继续使用：

> **纯材质 + 简单色板**

---

# 20. 黄色使用规则

黄色不是铺满整个建筑。

推荐层级：

```text
大面积：
深灰 / 枪灰

主要视觉外壳：
工业暗黄

机械连接：
银灰

功能：
青色

Warning：
少量橙色
```

远处应该看到：

> 黄 + 深灰

而不是：

> 一整块黄色塑料。

---

# 21. Bevel

Robot Center 属于 Q 版硬表面。

所有主要大型边缘都应有适当 Bevel。

推荐：

- 大件：明显圆角
- 小件：轻微圆角

避免绝对锋利的 90° CG 方块感。

但也不要把所有东西圆成玩具。

---

# 22. 面数原则

目前不是 Hero Character。

不要追求高模。

优先：

```text
轮廓 > 比例 > 功能识别 > 材质 > 小细节
```

能通过简单几何表达就不要增加拓扑。

---

# 23. Pivot

导出前检查 Pivot。

推荐：

### 整体结构

Pivot：

```text
底部中心
```

### Shop Display Pad

Pivot：

```text
平台中心
```

### Form Pad

Pivot：

```text
圆心
```

### Scanner

Pivot：

```text
扫描台中心或结构底部中心
```

便于 UE：

- 放置
- 对齐
- 做动画
- Spawn 特效

---

# 24. UE 功能预留

Blender 不写业务，但要给 UE 留明确位置。

需要预留：

```text
ShopInteractPoint
FormInteractPoint

ShopRobotDisplayPoint

FormPlayerStandPoint

FormFXCenter
```

不一定全部做 Socket。

可以在 Blender 用 Empty 标记位置，或者后续 UE 蓝图中设置 SceneComponent。

不要为了标记点做可见 Mesh。

---

# 25. 模块拆分建议

最终不要整个 Robot Center 只有一个 Mesh。

推荐拆成：

```text
SM_RobotCenter_Base

SM_RobotCenter_MainFrame
SM_RobotCenter_MainCore

SM_RobotCenter_ShopTerminal
SM_RobotCenter_ShopDisplayPad

SM_RobotCenter_FormPad
SM_RobotCenter_FormScanner
SM_RobotCenter_FormTerminal
```

大型不可动结构如果最终确认完全不需要动态，可后续合并。

现在优先保留功能模块边界。

---

# 26. 不需要拆开的东西

不要把：

```text
螺丝
小面板
一个黄色装饰条
一个支撑角
```

全部拆成独立 Object。

这些应：

- Join 到所属模块
- 或直接建在主体 Mesh 中

否则又会出现之前处理机那种部件数量爆炸问题。

---

# 27. 第一轮必须截图检查

Blockout 完成后暂时停止施工。

输出：

### 1. Front

```text
正视图
```

检查：

- 左右模块区别
- 主体宽度
- 横梁高度

### 2. Top

```text
俯视图
```

检查：

- 玩家动线
- Shop 与 Transform 占地
- 通行空间

### 3. 45° Perspective

这是最重要的一张。

检查：

- 整体是否像“一套设施”
- 是否过于复杂
- 是否过于像房子
- 是否过于像两个摊位

### 4. Human / OreBuddy Scale

至少一张带尺度 Proxy。

---

# 28. Blockout 验收条件

只有以下全部成立才继续：

- [ ] Robot Center 明显宽大于高。
- [ ] 整体是开放设施，不是室内建筑。
- [ ] 左边可以一眼认出是独立功能区。
- [ ] 右边可以一眼认出是独立功能区。
- [ ] 左侧主要使用方形语言。
- [ ] 右侧主要使用圆形语言。
- [ ] 中央主体足以把两边统一成一个设施。
- [ ] 正面坡道 / 台阶足够长且平缓。
- [ ] Human 可以自然走上平台。
- [ ] OreBuddy 尺度靠近平台时不显拥挤。
- [ ] 玩家可以自然走入 Transform Pad。
- [ ] Shop Display Pad 有足够空间摆机器人。
- [ ] 没有明显无意义细节。

---

# 29. 最终美术验收

最终模型必须符合：

```text
大形体
↓
功能结构
↓
少量机械细节
↓
灯光/警告细节
```

不能反过来。

正确：

> 第一眼：Robot Center。

第二眼：

> 左边机器人售卖，右边玩家幻化。

第三眼：

> 原来这里还有能源接口、扫描器、Warning。

错误：

> 第一眼全是螺丝、灯条、管线，但不知道这是干什么的。

---

# 30. Codex 施工纪律

施工过程中遵循：

## 必须

- 优先简单几何。
- 优先可读轮廓。
- 模块化。
- 保持命名规范。
- 保持 Transform 干净。
- 完成 Blockout 后先输出截图 Review。
- 没有 Review 不进入复杂细节阶段。

## 禁止

- 不擅自增加室内空间。
- 不增加墙和门。
- 不增加 NPC 区域。
- 不增加额外第三个功能摊位。
- 不增加复杂机械臂。
- 不增加机器人生产流水线。
- 不增加大量装饰管线。
- 不改变整体工业黄 + 枪灰 + 青色功能灯方向。
- 不为了“科幻”加入大量霓虹灯。
- 不为了“细节丰富”破坏大形体。

---

# 31. 推荐施工顺序

```text
01
新建 RobotCenter.blend

02
Reference Proxy

03
Base Platform

04
长而缓的正面通行坡道

05
Main Frame

06
Main Core

07
Shop Terminal

08
Shop Display Pad

09
Transform Pad

10
Transform Scanner

11
Transform Terminal

12
两根大型连接管

──────── STOP ────────

13
Front / Top / Perspective 截图

14
Review 大形体

──────── PASS 后 ─────

15
Bevel

16
二级机械结构

17
材质

18
青色功能灯位置

19
少量 Warning

20
清理 Object / Pivot / Naming

21
准备 UE 导出
```

---

# 最终核心原则

> **Robot Center 不是一栋可以进入的房子，而是一台大型开放式工业设备。**

> **左边是“购买一台机器人”，右边是“我自己成为机器人”。**

> **大轮廓比细节重要，功能识别比机械复杂度重要。**

> **先做 70% 正确的大形体，再补 30% 有意义的细节。**