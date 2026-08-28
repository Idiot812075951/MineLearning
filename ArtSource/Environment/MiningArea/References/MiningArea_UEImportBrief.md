# MiningArea UE Import Brief

## 1. 基本信息

- Asset Name：MiningArea Environment Module Library
- UE Name：保留当前源文件中的规范 `SM_MiningArea_*` 名称
- Asset Category：场景模块
- Priority：Current UE delivery
- Intended Folder：`/Game/MineLearning/Environment/MiningArea/Meshes/`
- Brief Status：Current
- Authoritative Blender File：`ArtSource/Environment/MiningArea/MiningArea_P2_Environment_Source.blend`
- Current Stage：UE 整理

## 2. 游戏用途

- 作为新地图可重复摆放的矿坑、地面、岩壁、坡面和大型岩体模块。
- 本文件不是最终 World Map，不在本轮决定 Robot Center、Warehouse、Processor 与道路的世界布局。

## 3. 风格目标

- 功能角色：矿区环境资产库。
- 身份与材质：保持 P2 源文件已经批准的岩土、矿坑和工业边界材料分区。
- 关键词：可复用、轮廓清楚、矿区层次明确。
- 避免：重做矿区、额外装饰、世界布局、写实废土化。

## 4. 技术交付

- Static Mesh。
- Blender 是静态造型、材质分区和比例的 Source of Truth。
- 仅导出实际存在、可复用且可验证的正式环境 Mesh。
- 排除 Camera、Light、Preview、Proxy、Review、Gameplay Empty 与点位。
- 不制作最终 Collision、LOD、材质、纹理、Blueprint、Landscape 或 NavMesh。
- 不永久应用会改变源设计的 Transform、Modifier、UV 或材质槽修改。

## 5. 本轮范围与保护项

- 必须完成：确认 P2 为最新源；逐模块 FBX；导入目标 UE `Meshes` 目录；验证尺度、朝向、Pivot、法线、材质槽和排除项。
- 必须保护：现有矿区造型、对象拆分、Pivot、UV、Modifier、材质槽及源 `.blend`。
- 停止点：Static Mesh 导入与检查完成后停止，不进入正式地图组装。

## 6. 验收重点

1. 只导入可复用环境 Mesh，没有辅助对象或玩法点位。
2. UE 尺寸、朝向、Pivot 与 Blender 数据一致，不依赖 UE 内补偿缩放。
3. 无反法线、破面、异常材质资产爆炸或错误合并。

已知风险：P2 是否已标记明确导出候选、整体矿坑是否需要保持为单一模块，须以只读场景检查结果为准。
