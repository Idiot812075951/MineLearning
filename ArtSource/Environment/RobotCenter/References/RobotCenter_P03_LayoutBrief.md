# Robot Center 第三轮布局优化施工清单

> 基于当前 `RobotCenter.blend` 第二轮 Blockout 继续修改。
> **本轮不推翻现有主体结构。**
>
> 当前参考图片仅用于理解：
>
> * 左侧机器人应该有独立离场方向
> * 右侧 Transform 应保持开放、低阻挡
>
> **图片仅供参考，不要求照图复刻。实际模型以本清单要求为准。**

---

# 1. 本轮目标

当前 Robot Center 的总体方向已经成立：

```text
中央 Main Frame / Main Core
        ↓
左：Robot Shop
右：Player Transform
```

本轮重点只解决两个实际玩法问题：

### 左侧

> 玩家购买机器人时，玩家和刚生成机器人的寻路不能互相阻挡。

### 右侧

> 玩家进入 Transform 区域、完成机器人幻化后，不应该受到扫描架、拱门、支柱等结构的碰撞和寻路限制。

因此本轮核心原则：

> **玩家交互路径与机器人部署路径分离。**

> **Transform 区域中心必须保持完全开放。**

---

# 2. 不要大改的部分

本轮以下部分原则上保持现状：

* `SM_RobotCenter_Base`
* `SM_RobotCenter_MainFrame`
* `SM_RobotCenter_MainCore`
* Robot Center 整体工业黄 + 枪灰 + Cyan 配色
* 中央主入口坡道
* 左 Shop / 右 Transform 的总体功能分区
* Main Core 与左右模块的视觉连接关系

除非左右模块调整后出现明显穿插，否则不要重新设计中央主体。

---

# 3. 左侧 Robot Shop：重新定义空间关系

左侧现在不能只理解成：

```text
Terminal
+
展示台
```

需要正式定义为：

```text
玩家操作区
+
机器人部署区
+
机器人离场通道
```

三个区域必须互不冲突。

---

# 4. 左侧推荐动线

目标俯视关系：

```text
                    Main Core

       Shop Terminal

           玩家
            ↓

      ┌────────────┐
      │ Deployment │
      │    Pad     │
      │     🤖     │
      └────────────┘
              \
               \
                └──────→ Robot Exit Ramp
                         ↓
                      正常地面
```

注意：

**机器人不是从 Deployment Pad 向中央主入口走。**

购买成功后应该直接朝 Robot Center 左侧外围离开。

---

# 5. Shop Terminal 位置

`SM_RobotCenter_ShopTerminal`

必须满足：

* 玩家站在终端前操作 UI。
* 玩家正常站位不能占据机器人 Deployment Pad。
* 玩家正常站位不能位于机器人离场路线中央。
* Terminal 应朝向 Robot Center 中央公共区域。
* 不需要玩家走到 Deployment Pad 上才能购买。

建议：

```text
Terminal
   ↑
Player

保留距离

Deployment Pad
```

即：

> 玩家操作机器人，而不是站在机器人出生位置操作机器人。

---

# 6. Shop Player Interact Point

重新检查：

```text
ShopInteractPoint
```

它应该位于：

> Shop Terminal 正前方 / 稍偏中央一侧。

禁止放在：

* Deployment Pad 中央
* Robot Exit Ramp 上
* 新机器人第一段 MoveTo 路径上

需要保证玩家站在 `ShopInteractPoint` 时：

```text
RobotSpawnPoint
→ RobotExitPoint
```

之间仍然存在完整通路。

---

# 7. Robot Deployment Pad

当前展示台正式定义为：

```text
Robot Display / Deployment Pad
```

不是纯装饰展示台。

建议命名仍可保持现有 Mesh 名称，但设计语义必须按部署台制作。

功能：

### 未购买时

可以用于：

* 展示预览机器人
* 显示当前选择机器人

### 购买成功时

真正机器人 Spawn 在这里。

因此 Deployment Pad 必须：

* 足够容纳 OreBuddy
* 未来也能容纳 Gunner / Carrier
* 四周不要被高结构包死
* 至少存在一个非常明确的开放离场方向

---

# 8. Deployment Pad 向左调整

本轮建议把 Deployment Pad：

> **整体略向 Robot Center 左外侧移动。**

目的不是为了对称，而是为了形成：

```text
Pad → 左侧 Ramp
```

天然连续路线。

Pad 不要再被 Terminal 和中央区域夹在中间。

理想视觉：

```text
中央公共区

  Shop Terminal

      Deployment Pad
             \
              \
             Left Exit Ramp
```

---

# 9. 新增左侧 Robot Exit Ramp

本轮最重要的新结构之一：

```text
SM_RobotCenter_RobotExitRamp
```

位置：

> Robot Center 左侧外缘。

用途：

> 专门让刚购买的机器人从 Deployment Pad 离开 Robot Center。

---

# 10. Robot Exit Ramp 要求

它应该是：

* 宽
* 缓
* 简单
* 工业物流坡道

不要做：

* 台阶
* 窄桥
* 护栏通道
* S 型道路
* 复杂转角

推荐路径：

```text
Deployment Pad
      ↓
轻微转向左侧
      ↓
Robot Exit Ramp
      ↓
正常 NavMesh
```

机器人从出生到离开平台最好只需要：

> **一次简单转向。**

不要要求机器人在建筑内部绕一圈。

---

# 11. Robot Exit Ramp 尺度

参考原则：

> 让当前项目最大常规机器人通过时，两边仍有余量。

不要只按照 OreBuddy 最小尺寸制作。

建议：

* 宽度约 2.5m~3m 量级作为初始参考
* 坡度与中央主坡道相近或更缓
* 两侧可以有低矮黄色工业包边
* 不做会影响 NavMesh 的高栏杆

实际尺寸仍以 Human / OreBuddy / 未来 Carrier Proxy Review 为准。

---

# 12. 左侧路径必须避免玩家阻挡

布局需要保证：

```text
Player
```

即使站在正常 Shop 操作位：

```text
RobotSpawnPoint
→ ExitRamp
```

仍然畅通。

这是本轮左侧最重要的验收项。

---

# 13. 程序点位预留

左侧至少保留：

```text
ShopInteractPoint

ShopRobotDisplayPoint
或
RobotSpawnPoint

RobotExitPoint

RobotJoinNavPoint
```

关系：

```text
Player
→ ShopInteractPoint

Robot
→ RobotSpawnPoint
→ RobotExitPoint
→ RobotJoinNavPoint
```

Blender 可以用 Empty 表示。

不需要制作可见 Mesh。

---

# 14. RobotExitPoint

放在：

> 左侧坡道入口或坡道中心。

它表达：

> 新机器人应该朝这个方向离开 Deployment Pad。

---

# 15. RobotJoinNavPoint

放在：

> 坡道下方、Robot Center 平台之外的正常地面区域。

用于以后 UE：

```text
Spawn Robot
→ MoveTo(RobotJoinNavPoint)
→ 进入正常工作 AI
```

如果最终 NavMesh 足够简单，可以直接 MoveTo JoinNavPoint。

因此 Blender 不需要设计复杂“轨道”。

---

# 16. 左侧不要增加生产车间

不要因为存在机器人出口，就开始增加：

* 封闭生产门
* 装配流水线
* 机器人仓库
* 机械臂
* 传送带

第一版语义直接定义为：

> Deployment Pad 就是机器人部署位置。

购买成功：

```text
机器人出现
↓
启动
↓
从左侧离场
```

简单即可。

---

# 17. 右侧 Transform：当前主要问题

现有圆形 Pad 方向正确。

真正需要修改的是：

> 上方 / 周围 Scanner 结构。

之前的半圆拱形 Scanner 存在三个问题：

1. 玩家进入时可能发生碰撞阻挡。
2. 玩家变成体积更大的机器人后可能卡在结构里。
3. Scanner 支柱可能切割 NavMesh / 玩家通行路径。

因此：

> **取消“玩家必须从拱门里面穿过去”的设计。**

---

# 18. Transform 新核心原则

Transform 必须变成：

# 开放式扫描平台

而不是：

# 扫描门 / 变身舱

中心区域必须：

```text
上方开放
+
前方开放
+
至少大部分侧面开放
```

---

# 19. 保留 Form Pad

`SM_RobotCenter_FormPad`

当前圆形识别是正确的。

继续保留：

* 圆形主体
* Cyan 功能环
* 工业黄色外圈
* 中央明确站位

不要推翻。

Transform 的视觉重点仍然应该是：

```text
○
```

与左侧 Shop：

```text
□
```

形成区别。

---

# 20. 删除 / 重做当前高拱形 Scanner

针对：

```text
SM_RobotCenter_FormScanner
```

不要继续强化现在这种：

```text
      ╭──────╮
     │       │
     │ Player│
     │       │
```

这种“门框”结构。

本轮改为：

> **低矮外围扫描节点。**

---

# 21. 推荐 Transform Scanner 新结构

推荐：

```text
俯视

          Scanner Node
               ▪


     ▪       ○       ▪

          Form Pad
```

或者：

```text
       后扫描器

           ▪

     ┌───────────┐
     │           │
侧 ▪ │    ○      │ ▪ 侧
     │           │
     └───────────┘

        玩家入口
```

核心是：

> Scanner 在 Pad 外围工作。

而不是：

> Scanner 架在玩家头上。

---

# 22. Scanner Node 数量

建议：

```text
3 个
```

即可。

例如：

* 左后
* 右后
* 后中

或者：

* 左
* 右
* 后

前方不要放高结构。

---

# 23. 前方必须完全开放

Transform Pad 朝中央公共区域的一侧：

> 必须保持无遮挡。

玩家动线：

```text
Main Entrance
      ↓
中央区域
      ↓
直接走上 Form Pad
```

之间不应存在：

* 门框
* 横梁
* 高柱
* 需要绕行的设备

---

# 24. Scanner Node 造型

每个 Node 不需要复杂。

可以是：

```text
矮机械柱
+
一个朝 Pad 中心的 Cyan Scanner Head
```

尺寸应：

> 低于玩家主要视觉遮挡高度。

或者至少不要形成完整立柱墙。

建议结构：

```text
  Cyan Head
     ◉
     │
  Gunmetal
  Short Base
```

Q 版、短粗、稳定。

---

# 25. Scanner Node 不要贴 Pad 太近

必须考虑：

> Human 变成比 Human 更大的机器人。

因此 Scanner 不能按照 Human Capsule 紧贴设计。

Pad 中央到 Scanner Node 之间应该留出明显安全空间。

设计基准不要只用 Human。

至少同时放：

```text
REF_Human
REF_OreBuddy
```

进入 Form Pad 检查。

如果未来 Carrier 明显更大，也要给它留扩展余量。

---

# 26. Transform 不需要物理“包住”玩家

幻化视觉可以以后在 UE 做：

* 垂直扫描光
* 环形光圈
* Niagara
* 地面能量
* Dissolve
* 上方投影光束

所以 Blender 结构没有必要为了表达“扫描”把玩家锁进机械笼子。

原则：

> **视觉效果负责包围感，模型结构负责开放通行。**

---

# 27. Transform Terminal

保留独立：

```text
SM_RobotCenter_FormTerminal
```

建议放：

> Form Pad 侧前方。

要求：

* 玩家靠近时容易看到
* 不挡进入 Pad
* 不占 Pad 中心
* 不位于玩家 Transform 后机器人离场的正前方

---

# 28. FormInteractPoint

建议：

> 位于 Form Terminal 前方。

玩家流程：

```text
靠近 Terminal
↓
打开 UI
↓
选择 Form
↓
玩家移动 / 或系统确认站在 Pad
↓
执行 Transform
```

如果最终玩法确定必须站在 Pad 上才能打开 UI，也仍然要求：

> Terminal 不构成碰撞瓶颈。

---

# 29. FormPlayerStandPoint

必须继续保持：

```text
Form Pad 正中心
```

并与：

```text
FormFXCenter
```

尽量完全重合。

---

# 30. Transform 后的机器人离开方式

与 Shop 不同：

Transform 不需要独立出口。

因为：

```text
Human
↓
站在 Pad
↓
Transform
↓
Robot Form
```

仍然是同一个玩家实体位置。

因此它可以：

> 从 Form Pad 正面直接离开。

这也是为什么：

# Transform 前方必须彻底开放。

---

# 31. 右侧不要做第二条专用坡道

目前不需要：

```text
Transform Exit Ramp
```

玩家使用：

```text
主入口坡道
```

进入和离开即可。

只有左侧购买机器人存在“额外实体生成”，才需要独立部署出口。

---

# 32. 左右空间逻辑最终应该形成

```text
                         Main Core

         SHOP                           TRANSFORM

   Terminal                              Terminal
      ↑                                    ↑
    Player                               Player
      │                                    │

 Deployment Pad                         Form Pad
      │                                    │
      ↓                                    ↓
新 Robot Spawn                      玩家自身变 Form
      │                                    │
      ↓                                    ↓
左侧 Exit Ramp                     正面直接自由离开
```

这是本轮最终空间逻辑。

---

# 33. 左右两边设计差异

## Robot Shop

关键词：

> 部署 / 输出 / 物流

因此：

```text
□ Pad
→ 侧向出口
```

## Transform

关键词：

> 站位 / 扫描 / 重构

因此：

```text
○ Pad
+ 开放式 Scanner Node
```

两个区域不仅造型不同，**行为也不同**。

---

# 34. 本轮禁止做

不要：

* 再增加 Transform 拱门
* 再增加 Transform 顶部横梁
* 做密封扫描舱
* 做玻璃舱
* 在 Form Pad 周围做一圈高围栏
* 为 Transform 添加独立出口道路
* 让购买机器人从中央主入口离场
* 让玩家站在 Deployment Pad 上购买
* 增加复杂机器人生产线
* 增加大量新机械细节

---

# 35. 本轮 Codex 执行顺序

```text
01
保留 Main Frame / Main Core

02
重新调整 Shop Terminal 与 Player 站位

03
Deployment Pad 向左外侧调整

04
新增左侧 Robot Exit Ramp

05
预留 RobotSpawnPoint

06
预留 RobotExitPoint

07
预留 RobotJoinNavPoint

08
检查 Player 站在 ShopInteractPoint
是否会挡 Robot 出厂路线

────────────

09
移除当前 Transform 高拱形 Scanner

10
保留圆形 Form Pad

11
设计 3 个低矮 Scanner Node

12
保证 Form Pad 正前方完全开放

13
保证 Human / OreBuddy 都不会与 Scanner 发生空间冲突

14
重新调整 Form Terminal

15
校正：
FormPlayerStandPoint
FormFXCenter
Pad Center
三者共轴

────────────

16
Front Review

17
Top Review

18
45° Review

19
Human Scale Review

20
OreBuddy Scale Review

21
Nav / Path Layout Review

STOP
```

---

# 36. 本轮必须增加一张 Path Review

除了之前截图，本轮额外输出一张：

# Top Path Review

使用俯视角。

只需要明确表示两个逻辑：

```text
Robot Shop：
Spawn Pad → 左侧 Exit Ramp

Transform：
中央区域 → Form Pad
```

不需要制作复杂黄色虚线。

可以：

* 用 Empty
* 简单箭头
* Blender Annotation

仅用于 Review，不作为最终模型。

---

# 37. 左侧验收条件

* [ ] 玩家站在 Shop 正常交互位时不会挡机器人离场。
* [ ] Deployment Pad 有明确开放出口方向。
* [ ] 新机器人不需要穿过 Robot Center 中央人群区域。
* [ ] 新机器人不需要经过中央主入口坡道才能离场。
* [ ] Robot Exit Ramp 足够宽且平缓。
* [ ] 从 Deployment Pad 到正常 NavMesh 路线简单。
* [ ] Robot Shop 仍然保持方形工业语言。

---

# 38. 右侧验收条件

* [ ] 删除高拱门式 Scanner。
* [ ] Form Pad 上方没有结构阻挡。
* [ ] Form Pad 正前方完全开放。
* [ ] Scanner Node 不构成围栏。
* [ ] Human 可以直线进入 Form Pad。
* [ ] OreBuddy 尺度在 Pad 上不会卡 Scanner。
* [ ] 玩家 Transform 后可以直接向前离开。
* [ ] Transform 仍然一眼能识别为扫描 / 重构区域。
* [ ] Form Pad 继续保持明显圆形语言。

---

# 39. 本轮最重要的判断标准

不要问：

> “看起来是不是比之前更复杂？”

要问：

### Robot Shop

> **我买一台机器人后，它能不能自然地自己走出去，而不是和我抢路？**

### Transform

> **我从 Human 变成任意机器人后，这个装置会不会反过来限制我的体型和行动？**

如果答案都是：

> 不会。

本轮布局就成功。

---

# 最终原则

> **Shop 的玩家交互面朝中央，机器人物流面朝外围。**

> **Transform 的机械结构围绕玩家，但绝不封住玩家。**

> **模型不要靠“门”表达幻化，利用地台 + 低矮扫描节点表达扫描即可。**

> **图片只作为空间关系参考，不要求按照图片中的具体造型、比例或结构复刻。**
