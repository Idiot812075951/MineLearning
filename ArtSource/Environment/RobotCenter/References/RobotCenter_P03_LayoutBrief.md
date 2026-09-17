# Robot Center — 当前布局要求 P03

唯一源文件：`ArtSource/Environment/RobotCenter/RobotCenter.blend`。
概念参考为同目录 `RobotCenter_Concept.png`，布局语义参考为 `RobotCenter_P03_PathReference.png`；后者是标注功能动线的设计参考，不是验收截图。

保留中央 Main Frame / Main Core、左侧 Robot Shop、右侧 Player Transform 的整体结构。保持 MineLearning 圆润工业科幻风格、公共材质、稳定 Pivot、功能点位和已确认比例。

左侧将玩家交互区、机器人部署台和机器人离场方向分开。Shop Terminal 不挡部署与离场路径；空部署台不放固定机器人。机器人生成后经专用左侧出口坡道接入导航，避免与站在终端的玩家相互阻挡。保留 `RobotExitPoint`、`RobotJoinNavPoint` 等程序点位语义。

右侧为开放扫描平台，不是必须穿过的拱门或封闭变身舱。Form Pad 保持中心和前方开放，扫描结构分布在侧后方；终端、交互点和 `FormPlayerStandPoint` 不阻挡玩家及变身后机器人。右侧不另建专用出口坡道。

不加入生产车间、额外玩法或固定角色。后续导出只包含正式环境网格，排除预览、摄像机、灯光和玩法辅助点；进入 UE 前重新核对尺寸、法线、材质槽、Pivot 与命名。

只保留当前文件和本说明，不保留阶段备份、制作脚本与验收图片。
