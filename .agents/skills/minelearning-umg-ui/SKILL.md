---
name: minelearning-umg-ui
description: Design, implement, modify, debug, or review runtime UI for the MineLearning Unreal project. Use for HUDs, menus, prompts, crosshairs, skill bars, ListViews, widget architecture, UI input behavior, and UI data flow. Enforces UMG Widget Blueprint-first presentation, event-driven updates without Tick, business/UI separation, configurable visuals, and Unreal UI lifecycle conventions. Do not use for gameplay-only work with no UI or editor-only tooling.
---

# MineLearning UMG UI 规范

## 最高约束

- 项目运行时 UI 必须使用 UMG。视觉树通常由 Widget Blueprint 制作，不允许使用 Slate 直接绘制游戏 UI。
- 禁止用 `SNew`、`SOverlay`、`SCompoundWidget`、`AddViewportWidgetContent` 或手写 Slate Paint 代替 UMG。发现任务范围内已有的 Slate 游戏 UI 时，不继续扩展它；将受影响部分迁移到 UMG。
- UI 的视觉表现及其响应逻辑必须写在 Widget Blueprint 中，包括监听状态事件、更新文本/图标/颜色/显隐、播放 UI 动画和刷新叶子控件。业务 C++ 只提供权威状态、查询接口和状态变化事件，不操作具体 UI。
- UI 是表现层，不是业务权威。它接收已经算好的显示数据并呈现，不负责计算玩法结果。
- UI 不使用 Tick。数据变化必须由明确事件驱动，并保证绑定与解绑成对。
- 视觉内容必须可替换、可配置。不得把未来可能变化的图标、文本、颜色或布局语义硬编码成临时字符或绘制代码。

用户当前需求、项目 `AGENTS.md` 和既有明确约定优先。与 `$ue-coding-nindo` 同时适用时，继续遵循“程序一定要简单，实现一定要聪明”，不要为了 UI 架构额外制造无实际用途的层级。

## 职责与数据流

强制的数据流是：

`业务状态拥有者 -> 状态变化事件 -> UMG Widget Blueprint/界面宿主 -> 叶子 Widget`

- Gameplay 对象负责计算、校验和修改业务状态；Widget 只接收适合显示的值，例如剩余弹药、是否可用、提示文本和图标资源。
- Gameplay 对象不得创建、查找、持有、转换或调用 Widget、WidgetComponent、具体控件或其他 UI 类型；即使调用的是 `SetAmmoCount` 一类公开展示接口，也属于业务层反向依赖表现层，禁止使用。
- 状态拥有者只广播已经发生的事实和最终权威值。Widget Blueprint 或独立界面宿主主动取得数据源、绑定事件、执行一次初始刷新，并在事件到达时更新自身；UI 不存在时不得影响 Gameplay 运行。
- Widget 不自行搜索 World、遍历 Actor、查询距离、推导技能可用性、执行伤害、修改库存或发起权威玩法操作。
- UI 发出的点击、选择和确认只能表达用户意图；业务对象决定操作是否成立，再通过事件把结果反馈给 UI。
- 同一业务数据只保留一个真相来源。不要在 Widget 中复制一套需要与 Gameplay 同步的业务状态。
- 叶子 Widget 必须严格保持纯展示。`ListView` Entry 只消费条目提供的展示数据；在条目被复用时完整刷新所有可见状态，不能依赖上一个条目的残留值。
- 非通用的页面级 Widget 可以负责少量界面编排和把显示数据分发给子 Widget，但不得因此承载玩法计算。需要放宽时，先确认逻辑确实只属于该界面的表现流程。

不要为了形式引入 Presenter、ViewModel、Subsystem 或新接口。只有它能形成真实复用、稳定数据边界或正确生命周期时才增加一层；简单界面由 Widget Blueprint 直接订阅状态拥有者的事件即可，但状态拥有者仍然不得认识或调用 Widget。

## UMG 与 Blueprint/C++ 边界

- Widget Blueprint 负责层级、布局、锚点、图片与 Brush、字体、样式、动画、资源装配，以及由状态事件驱动的控件更新和 UI 表现流程。
- 优先复用现有 Widget Blueprint、样式和项目 UI 组件；不要为单个界面复制近似控件。
- C++ 可提供权威业务状态、只读查询、`BlueprintAssignable` 状态事件、展示数据类型，以及创建 Widget、设置 Owning Player 等必要的生命周期装配；这些代码不得读取或修改具体视觉控件。
- C++ 不实现 UI 表现响应，不通过名称查找控件，不设置 Text/Image/Brush/颜色/显隐，不播放 UI 动画，不构建运行时 UI 的视觉树，也不用 Slate 绘制项目游戏 UI。需要更新显示时，由 Widget Blueprint 监听事件并更新自身。
- 不因为“以后可能复用”提前创建庞大 UI 框架。先完成当前界面的最小闭环，再从真实重复中提取通用能力。
- CommonUI、UMG Viewmodel/MVVM 等系统只有在项目已经采用，或当前需求确实需要平台输入、导航或规模化绑定能力时才引入。

## 事件驱动与生命周期

- 禁止 Widget Tick、轮询 Timer、循环 Delay，以及用动画 Tick 代替状态事件。
- 不使用会每帧求值的 UMG 属性绑定来更新运行时数据；由 Widget Blueprint 的事件处理流程更新具体控件或调用叶子 Widget 的明确展示接口。
- 先绑定状态变化事件，再执行一次初始刷新，避免首次显示空白或错过绑定前的状态。
- 外部数据源变化时，先解绑旧对象，再绑定新对象。Widget 销毁、移除、停用或所有者切换时必须解绑。
- C++ 原生 Delegate 保存并移除 `FDelegateHandle`；动态代理使用与生命周期匹配的唯一绑定和显式解绑，避免重复回调。
- 不假设 `Construct` 只执行一次。一次性初始化使用合适的初始化阶段；可能反复进入视口的绑定必须能够安全重复进入和退出。
- 异步加载、动画完成、列表刷新和输入监听同样遵循注册/注销、开始/取消的成对生命周期。

## 可配置与通用性

- 图标应是可配置的 Brush、Texture、Material 或 Widget 类型。准心不能写死为 `"+"` 字符，技能图标不能写死进绘制代码。
- 颜色、禁用态、尺寸、间距和字体优先来自 Widget Blueprint 默认值、样式或已有项目配置；只有业务含义稳定的值才进入代码。
- 对外暴露最小而清楚的显示接口，例如设置图标、数量、状态和提示。不要把内部控件全部设为公开可写。
- 通用控件通过参数和展示数据覆盖真实变体；不要用不断增长的布尔开关兼容互不相关的页面。
- 可替换资源必须有合理默认值，并对缺失资源提供可诊断但不破坏输入的降级表现。

## UE UI 与输入规范

- 创建运行时 Widget 时传入正确的 Owning Player。玩家独享 UI 优先使用与本地玩家对应的屏幕添加方式；全局覆盖层才使用全局视口语义。
- 被动显示 UI（准心、状态图标、提示文本等）使用不会命中测试的 Visibility，不得抢占焦点、消费鼠标输入或改变输入模式。
- 只有真正需要鼠标交互的菜单或界面协调者才能请求 UI 输入模式；必须在关闭时恢复此前的输入状态。叶子 Widget 不修改鼠标捕获、锁定、光标显示或 PlayerController 输入模式。
- 尊重锚点、DPI Scaling、Safe Zone、不同宽高比和本地化。玩家可见文本使用 `FText`，不靠字符串拼接构造可翻译句子。
- 可交互控件提供清楚的 Focus、Hover、Pressed、Disabled 状态，并保持键鼠与手柄导航可达。纯装饰控件不参与 Hit Test 或 Focus。
- 动画只表达界面状态，不作为业务完成条件或业务计时器；业务结果不能依赖某个 Widget 仍然存在。
- 遵循当前 UE 版本及项目已有的 UMG 生命周期、命名和资源组织方式；不要用旧教程中的临时做法覆盖当前项目约定。

## 工作流程

1. 只读检查相关 Widget Blueprint、创建者、Owning Player、添加/移除视口的位置、输入模式和业务状态来源。
2. 明确 UI 要显示的最小数据契约、变化事件、初始刷新时机以及销毁/换主时的解绑点。
3. 在 Widget Blueprint 中完成视觉树、可配置资源和表现响应；由 Widget Blueprint 或独立界面宿主订阅状态事件，禁止由状态拥有者发起 UI 更新。
4. 只修改当前需求需要的 UI 路径。若遇到 Slate 游戏 UI，迁移受影响的部分，不顺手重做无关页面。
5. 验证 Widget Blueprint 编译、实际添加到正确玩家视口、首次状态、事件更新、重复打开关闭、输入不被遮挡，以及目标分辨率和 DPI 表现。

## 完成门槛

完成或评审 UI 工作前确认：

- 运行时视觉是否全部由 UMG Widget Blueprint 构建，没有 Slate 游戏 UI。
- UI 的事件监听、控件刷新和动画表现是否位于 Widget Blueprint，而不是 Gameplay C++。
- Gameplay 对象是否完全不知道具体 UI，没有 Widget 引用、控件名称、类型转换或展示接口调用。
- Widget 是否只显示外部提供的数据，没有隐藏的玩法计算或 World 查询。
- 是否不存在 Tick、轮询 Timer 和每帧 UMG 属性绑定。
- 所有 Delegate、异步回调和输入监听是否按生命周期解绑。
- 图标和视觉变体是否可配置，而不是字符、路径或 Paint 逻辑硬编码。
- 被动 UI 是否不命中测试、不抢焦点、不改变鼠标或相机操作。
- UI 是否在正确 Owning Player 的视口中真实可见，并验证过重复进入、退出和状态切换。

不能用“代码已调用 AddToViewport”代替可见性验证；不能用“自动化测试通过”代替至少一次与输入、布局或动画风险相称的实际 UI 验证。
