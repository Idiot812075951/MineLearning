# 2026-07-30 提交风险与 TODO

## 已知风险

- `BP_OreFiedManager` 是重命名 `BP_OreFieldManager` 后留下的 Redirector。当前关卡仍可能通过 Redirector 解析，后续应在 UE Content Browser 中执行 Fix Up Redirectors，并重新保存关卡。
- 新机器人部分资产仍包含 `Test`、`Collect_Test` 等阶段性命名，功能已验证，但正式资产命名尚未统一。
- `BP_Orebuy07Controller` 存在拼写不规范，后续重命名时必须通过 UE 完成并修复 Redirector，避免直接移动 `.uasset`。
- 矿斗预览在运行时创建 `UInstancedStaticMeshComponent`，依赖 `S_CargoBin` 插槽、项目自有材质及 Instanced Static Mesh 材质 Usage；需要在打包版本中再次验证。
- 机器人拾取吸附、交付点旋转和 Montage Notify 均依赖蓝图资产配置，错误的 NotifyName 或 SocketName 会导致动作播放但业务未触发。
- 当前矿区生成点采用场景全局查找；多个矿区管理器共存时会共享全部生成点，尚未增加分组机制。
- 当前矿石血条由 C++ 动态构造，仅覆盖基础显示，尚未进行距离裁剪、遮挡和批量性能验证。
- 本轮功能以 PIE 人工验收为主，尚无自动化测试覆盖挖矿、拾取、交付、加工和矿区清空闭环。

## 后续 TODO

- 在 UE 中执行 Fix Up Redirectors，清理 `BP_OreFiedManager` 等重命名残留。
- 统一机器人动画、骨骼、控制器和蓝图资产命名，移除阶段性 `Test` 后缀。
- 增加打包构建验证，重点检查矿斗实例材质、WidgetComponent 血条和 Montage Notify。
- 为矿区生成点增加可选分组，避免多个矿区管理器争用同一批 SpawnPoint。
- 补充最小自动化测试：库存预留与提交、矿石耗尽仅广播一次、加工取消释放预留资源。
- 按发布需求评估矿石血条的显示距离、遮挡策略和同屏数量性能。

## 本轮保留的异常日志

- Montage 或 AnimInstance 配置缺失。
- ResourceDepot 缺失、MoveTo 交付点失败或提交失败。
- ResourceProcessor 仓库配置、预留提交、GameInstance、Subsystem 或 PlayerData 缺失。

以上日志均用于定位真实配置错误，不属于正常流程测试打印。
