# Guren Q 编辑器施工记录

最终资产已保存在 UE 项目中，打开地图直接运行即可，无需重跑施工脚本。

- `qa_cases.py`：PIE 内近远距离、无目标、飞行、打断、目标销毁和低帧率检查，会消耗当前 PIE 中的测试目标。
- `qa_materials.py`：PIE 内检查辐射阶段打断后的材质、绑定和控制恢复。
- `visual_radiation.py`：PIE 内慢速播放并暂停在辐射阶段，供观察。
- `finalize_editor.py`：停止 PIE 后保存本次相关资产，恢复临时测试设置。
- 其余脚本为一次性调查和施工记录，依赖当时的图节点状态，**不要作为可重复安装脚本运行**。尤其 `setup_scene.py`、`bind_presentation.py`、`finish_graphs.py` 会修改既有图连线。

检查结果位于项目 `Saved/GurenQ/`。Blender 导出脚本、FBX 和源状态备份位于 `ArtSource/Characters/GurenSeitenHakkyoShiki/UEExport/QSkill/`。
