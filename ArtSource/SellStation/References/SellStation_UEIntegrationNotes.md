# Sell Station UE Integration Notes

本文件记录本轮 Blender 静态资产为后续 UE 接入预留的结构，不代表已经修改 UE 资产或业务逻辑。

## 建议组件结构

```text
BP_SellStation
├── Body                  -> SM_SellStation_Body
├── Screen                -> SM_SellStation_Screen
├── ItemDisplayPoint
├── TransactionPoint
├── CoinSpawnPoint
└── RobotApproachPoint
```

- `Body` 与 `Screen` 共享 Blender 世界地面中心 Pivot；作为两个 Static Mesh 组件装配时相对 Transform 可保持归零。
- `Screen` 只包含 `M_SS_ScreenDynamic`，便于创建 MID 或后续替换为 Render Target / Widget 驱动材质。
- Blender Empty 坐标（米）：
  - `ItemDisplayPoint`：`(0.00, -0.07, 0.85)`
  - `TransactionPoint`：`(0.00, -0.07, 0.85)`
  - `CoinSpawnPoint`：`(0.00, -0.07, 0.91)`
  - `RobotApproachPoint`：`(0.00, -1.55, 0.00)`
- UE 轴转换后应以导入结果和项目 Forward 约定复核，不直接复制数值后跳过可视检查。

## 建议运行时流程

1. 机器人到达 `RobotApproachPoint`，把待售 Item / Pickup 数据提交给出售台。
2. 出售台在 `ItemDisplayPoint` 生成仅用于表现的货物 Mesh；关闭其碰撞和物理，避免干扰接近与交易。
3. 交易确认后锁定本次请求，在 `TransactionPoint` 播放 Niagara / Material 反馈，并移除货物表现。
4. 屏幕 MID 显示交易状态和本次金币数；模型本身不包含文字、数字或图标。
5. 在 `CoinSpawnPoint` 按交易结果生成 Coin Pickup。金币由后续真实消费系统处理，不在模型或出售台静态资产中保存固定金币。
6. 交易结束后恢复 Idle 状态，允许下一次 Item 数据进入。

## 碰撞与寻路建议

- 主体只保留覆盖底座、前角护件和后方屏幕支撑的简单碰撞。
- 中央货面上方保持空，动态货物表现默认无碰撞。
- 不要给屏幕显示平面单独添加阻挡碰撞。
- `RobotApproachPoint` 必须落在有效 NavMesh 上；前方中央 190 cm 以上宽度保持无遮挡。

## 本轮未实现

- UE Blueprint、Item / Pickup 数据接线、MID / Render Target、Niagara、Coin Pickup 生成与交易业务逻辑。
- FBX 导出、正式碰撞、UV/Lightmap 最终整理和 UE 导入验证。
