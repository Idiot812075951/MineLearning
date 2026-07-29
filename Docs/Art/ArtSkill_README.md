# MineLearning Art Skill v1

这是一套供 Codex + Blender/MCP 使用的项目级美术生产约束。

## 目录结构

```text
MineLearning/
├─ AGENTS.md
├─ ArtBriefs/
│  └─ SM_Ore_Iron_01.md
└─ .agents/
   └─ skills/
      └─ mining-game-3d-asset-production/
         ├─ SKILL.md
         ├─ references/
         │  ├─ art-direction.md
         │  └─ acceptance-rubric.md
         └─ assets/
            └─ asset-brief-template.md
```

## 安装

把本压缩包中的内容复制到 **MineLearning Git 仓库根目录**。

最终应看到：

```text
<你的 MineLearning 仓库>/.agents/skills/mining-game-3d-asset-production/SKILL.md
```

### 已存在 AGENTS.md 时

不要直接覆盖项目原有 `AGENTS.md`。

把本包 `AGENTS.md` 中的“美术资产工作”章节合并到现有文件，保留原有代码、构建和测试规则。

### Codex 没发现 Skill 时

1. 确认 Codex 当前工作目录位于 MineLearning 仓库内。
2. 在 Codex 中运行 `/skills`，或输入 `$` 查找：
   - `mining-game-3d-asset-production`
3. 若刚复制文件后仍未出现，重新启动当前 Codex 会话。
4. 检查目录和文件名大小写：
   - `.agents/skills/.../SKILL.md`

## 第一次建模任务

确认 Blender MCP 已连接，并让 Codex 打开/使用包含现有项目资产的 Blender 文件。

然后直接发送：

```text
请显式使用 $mining-game-3d-asset-production。

读取 ArtBriefs/SM_Ore_Iron_01.md，并严格按 Brief 执行。
先检查当前 Blender 场景和已有资产，不得清空或破坏 OREBUDDY-07。
本轮只完成 Phase 0 和 Phase 1：制作三个铁矿石草模候选、按验收表自评、推荐一个候选并保存可回退版本。
不要进入精修、最终材质、UV、碰撞或 FBX 导出。
完成后汇报三个候选的差异、逐项评分、推荐理由、保存文件位置和已知问题。
```

## 推荐工作方式

- 第一次和前几次任务都显式写 `$mining-game-3d-asset-production`，不要只依赖自动触发。
- 每个资产复制 `asset-brief-template.md` 到 `ArtBriefs/` 后填写。
- 第一轮只做草模；确认方向后，在下一轮明确批准进入 Phase 2。
- 每次发现 AI 重复犯错，把规则补进最接近问题的位置：
  - 永久底线：`AGENTS.md`
  - 工作流程：`SKILL.md`
  - 审美问题：`art-direction.md`
  - 可判定的失败标准：`acceptance-rubric.md`
  - 单个资产特殊要求：对应 Asset Brief
- 建议把这些文件提交到 Git，让每个 Codex 任务都使用同一版本。

## 建议首个提交信息

```text
chore(art): add stylized 3D asset production skill
```
