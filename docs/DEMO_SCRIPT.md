# 5～8 分钟课堂演示脚本

## 0:00～0:45 项目定位与画面

1. 运行 `run.bat`。
2. 说明这是 C++17/OpenGL 3.3 Core 教学可视化，不是生产控制软件。
3. 用默认全景指出煤壁/巷道、30 台液压支架、双滚筒采煤机、刮板输送机、通风管和工作灯。
4. 按 `F1` 快速展示完整快捷键，再关闭帮助。

## 0:45～2:15 正常联动流程

1. 在 `Longwall Control Station` 点击 `Initialize`。
2. 指出状态从 `Initializing` 到 `Ready`，日志记录转换原因和时刻。
3. 点击 `Start`。说明状态先进入 `Starting conveyor`，输送机达到速度后才允许采煤机割煤。
4. 按 `2` 切到采煤机近景，观察移动、滚筒旋转和煤尘/煤块。
5. 从 `Windows → Equipment` 打开设备列表，选择 Shearer，展示位置、方向、速度、负载、温度的平滑变化。

## 2:15～3:25 输送机与支架

1. 按 `4` 查看输送机，调整 `Speed command`。
2. 指出刮板相位和煤块运输；未割煤时不会无限生成煤块。
3. 按 `3` 看支架侧视，选中不同编号。
4. 观察采煤机通过后支架依次 `Waiting / Lowering / Advancing / Raising / Normal`，而非同时动作。

## 3:25～4:15 图形学功能

1. 依次切换 `Distance fog`、`Headlamp`、`Work lights`。
2. 打开 `Wireframe` 展示 Core Profile 网格；再关闭。
3. 选中设备并打开 `Bounds`，说明鼠标射线与 AABB 拾取及橙色高亮。
4. 按 `6` 展示移除顶板的教学顶视剖面，按 `1` 回全景。

## 4:15～5:30 故障

1. 注入 `Conveyor jam`，说明输送机速度归零、采煤机停止、状态进入 Fault。
2. 打开 Windows → Event log，指出时间、等级、来源和内容。
3. 清除堵塞；系统保持安全停止，需重新 Initialize/Start。
4. 注入 `Fail work-face lights`，观察工作灯真实关闭；相机头灯仍独立可用。
5. 选择一台支架，注入低压，观察其压力和高度变化。

## 5:30～6:30 急停与恢复

1. 正常运行时点击红色 `EMERGENCY STOP`。
2. 指出全部运动立即停止，状态为 `EMERGENCY STOPPED`。
3. 尝试 Reset，说明锁存未释放时被拒绝并写 Safety 日志。
4. 点击 `Release latch`，再 `Reset`，然后可重新 Initialize。

## 6:30～7:30 架构与测试

1. 展示 `docs/ARCHITECTURE.md`：渲染只读设备快照，Simulation 不调用 OpenGL。
2. 说明 Cube/Cylinder/Rock Mesh 复用、Model/View/Projection、法线矩阵和 RAII。
3. 展示 `ctest --test-dir build -C Release --output-on-failure` 的通过结果。
4. 按 `F12`，说明截图来自 OpenGL back buffer，输出到 `screenshots/latest.bmp`。

## 7:30～8:00 限制

说明当前未实现 Shadow Mapping、GPU Instancing、外部纹理/模型和工业级参数；这些被诚实记录为 P1/P2 限制，没有用空实现冒充。
