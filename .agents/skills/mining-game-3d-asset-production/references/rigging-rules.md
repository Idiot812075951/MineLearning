# MineLearning 机械 Rig 规则

仅在任务涉及 Armature、Bone、机械关节、Vertex Group、刚性绑定、约束或 Skeletal Mesh 时读取。

## 修改前基线

- 确认正确的 `.blend` 路径、当前源版本和可回退文件。
- 记录 Armature、Bone 层级、Root、对象父子关系、Armature Modifier、Vertex Group、约束、现有 Action 和关键对象 Transform。
- 若无法判断某对象由父子关系、Bone Parent、Constraint 还是 Armature Modifier 驱动，停止修改并报告。
- 已有 Rig 任务默认保护 Bone 名称、Rest Pose、局部轴、Vertex Group 和 Action，除非 Brief 明确授权修改。

## 刚性机械绑定

- 活动部件应按真实功能拆分，具有合理 Pivot、局部轴和可解释名称。
- 刚性机械件优先使用可验证的 Bone Parent、刚性 Vertex Group 或明确约束；不得把未经检查的自动权重当作最终绑定。
- 刚性权重应避免跨关节渐变和相邻零件串权。若使用 Armature Deform，验证目标部件由预期 Bone 驱动。
- 不为简化导出随意 Join Mesh、Apply Armature、合并活动件或改变父子关系。
- 不为“看起来能动”建立冗余 Bone、循环约束或难以维护的 Driver。

## 机械关节

- Pivot 位于实际旋转或伸缩中心，局部轴与预期运动方向一致。
- 旋转、伸缩和滑动范围应符合结构，不允许明显穿插、脱节或悬空。
- 对称部件保持一致的命名、轴向和层级逻辑，但不要用负缩放制造不可控导出结果。
- Root 与主体层级必须稳定；不要把局部机械动作意外写入 Root。

## 修改后验证

- 对比修改前后的 Bone 数量/名称/父级、Armature Modifier、Vertex Group、对象 Transform 和受保护 Action。
- 在 Bind/Rest 状态以及相关动作的首、中、末帧检查关节、工具、履带和 Cargo 等活动结构。
- 检查非目标对象是否保持静止，刚性零件是否无弯折，约束是否无循环或跳变。
- 恢复原活动 Action、帧和模式，再保存阶段版本。

若继续处理必须重做 Rest Pose、大范围重权重或依赖人工修复，停止并报告更小的替代方案。
