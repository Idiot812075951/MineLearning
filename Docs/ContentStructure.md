# MineLearning Content 目录规范

项目自有资产统一放在 `/Game/MineLearning` 下，按功能域组织；不要再向
`/Game/Blueprint`、`/Game/FX`、`/Game/Resource` 或模板目录散放项目资产。

## 标准目录

```text
/Game/MineLearning
├── Characters
│   ├── Gunner
│   │   ├── Animations
│   │   ├── Blueprints
│   │   ├── FX
│   │   ├── Materials
│   │   ├── Meshes
│   │   ├── Textures
│   │   └── Weapons/AK
│   └── OreBuddy
│       ├── Animations
│       ├── Blueprints
│       ├── Materials
│       └── Meshes
├── Gameplay
│   └── Weapons/Blueprints
├── Input
│   └── Actions
├── Mining
│   ├── Blueprints
│   ├── Data
│   ├── Ores/Iron
│   │   ├── Blueprints
│   │   ├── FX
│   │   ├── Materials
│   │   ├── Meshes
│   │   └── Textures
│   └── UI
├── Player
│   └── Blueprints
├── ThirdParty
└── UI
```

## 保留目录与例外

- `/Game/Characters/Mannequins`、`/Game/Characters/Mannequin_UE4` 和
  `/Game/LevelPrototyping` 是 Epic 模板内容，保持原目录，便于升级和溯源。
- `/Game/ThirdPerson/Maps/ThirdPersonMap` 暂时保留；它是 World Partition 地图，
  其 `__ExternalActors__` 路径与关卡包名绑定，不能作为普通文件夹直接移动。
- `/Game/ThirdPerson/Blueprints/Resource/BP_OreFieldManager` 暂时与该地图一同
  保留。World Partition 外部 Actor 直接绑定此类；除非连同地图做专门迁移和回归，
  不得为了目录整洁单独移动它。
- 外部导入内容先放入 `/Game/MineLearning/ThirdParty/<Vendor>`；确认要二次加工的
  资产再迁入对应功能域，避免第三方内容与项目资产混杂。

## 命名规则

- Blueprint：`BP_`；动画蓝图：`ABP_`；Widget：`WBP_`。
- Static Mesh：`SM_`；Skeletal Mesh：`SK_`；Skeleton：`SKEL_`；
  Physics Asset：`PHYS_`。
- Animation Sequence：`AS_`；Animation Montage：`AM_`。
- Material：`M_`；Material Instance：`MI_`；Texture：`T_`。
- Niagara System：`NS_`；Input Action：`IA_`；Input Mapping Context：`IMC_`。
- 资产名使用稳定的英文语义名；不使用日期、测试编号、GUID 或导入工具临时名作为
  最终名称。

## 迁移约束

1. 只通过 Unreal Editor 的 Rename/Move 移动 `.uasset`，禁止在资源管理器中剪切。
2. 每次按单个功能链小批迁移，保存并编译相关 Blueprint 后再继续。
3. C++ 中的 `/Game/...` 硬路径必须先更新、完整编译并重启编辑器，再移动目标资产。
4. 全部迁移完成后执行 Fix Up Redirectors，并做一次干净编译、编辑器重启和 PIE 回归。
5. 合入前检查旧根目录，除必要重定向器外不得新增项目资产。
