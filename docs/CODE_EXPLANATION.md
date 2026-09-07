# 代码说明

## OpenGL 初始化

`Application::initializeWindow` 请求 OpenGL 3.3 Core Profile 和 4x MSAA，创建 GLFW 窗口后用 GLAD 2 加载函数。Renderer 启用深度测试、背面剔除和 Alpha 混合。每帧使用真实 framebuffer 尺寸设置 viewport，因此 Windows DPI 或窗口缩放不会拉伸投影。

## VAO、VBO 与 EBO

`Mesh` 的顶点为位置 3 浮点、法线 3 浮点、UV 2 浮点。构造函数一次上传 VBO/EBO 并记录三个 attribute；Cube、Cylinder 和低多边形 Rock 缓冲区只创建一次，所有环境和设备通过不同 Model 矩阵复用。析构函数释放三个句柄，类不可复制、可移动，防止双重释放。

## Shader、MVP 与法线矩阵

顶点着色器执行：

```glsl
gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
```

局部顶点先变换到世界，再到观察和裁剪空间。非均匀缩放使用 `transpose(inverse(mat3(model)))` 变换法线，避免光照方向失真。Shader 编译/链接错误包含具体文件和 OpenGL 日志；Windows 中文路径通过 `std::filesystem::u8path` 打开。

## 光照、材质与雾

片元着色器组合低强度环境光、六个沿工作面分布的点光源和相机锥形头灯。点光源具有线性与平方衰减；镜面项采用 Cook–Torrance 微表面模型，以 GGX 分布、Smith 几何遮蔽和 Schlick 菲涅耳计算；金属度用于分配漫反射与镜面反射。相机附近两盏工作灯先渲染深度纹理数组，再做 3×3 PCF 比较，远处灯保持补光。Uniform 位置按已链接程序缓存。材质由颜色、金属度、粗糙度、发光、透明度和程序化表面类型组成。

距离雾使用指数平方：

```glsl
1.0 - exp(-density * density * distance * distance)
```

最终雾量限制到 0.92，远处仍保留轮廓。照明故障 Uniform 会关闭工作灯，头灯保持独立，故障因此真实改变画面。

## 程序化场景

Renderer 用 Box/Cylinder 组合分层底板、顶板、煤壁、巷帮、钢拱架、灯具、分段通风管和电缆。30 台支架复用同一 Mesh，由分体底座、伸缩立柱、顶梁、掩护梁、连杆、管路和阀组组成。采煤机由长低机身、设备舱、左右摇臂、液压缸、左右滚筒和滚筒截齿组成；滚筒角来自 Shearer 仿真对象。输送机由分段槽体、双链、齿轨、机头/机尾与按 `chainPhase` 平移的刮板组成，煤块使用低多边形 Rock Mesh，落点高度按槽板顶面与煤块尺寸计算。交错链环和刮板一起向负 X 机头移动。滚筒轴朝向煤壁，换向时左右摇臂连续交换高低位置。水雾与煤尘从两端滚筒生成，以深度排序的透明广告牌绘制。

## 动画与设备联动

Shearer 使用目标速度加速度收敛，而不是直接跳速；负载由位置的连续正弦函数决定，温度按负载平滑追踪，因此遥测确定且不逐帧随机。到达边界后位置钳制、方向反转并产生一次性 endpoint 事件，Controller 进入 `EndTransition` 后延时恢复。

支架并不一起动作。采煤机按方向越过每台支架 1.8 m 后，Controller 只触发该支架；各支架还有延迟和三阶段定时。视觉位姿使用 Smoothstep 串接降架、0.18 m 移架和升架，各阶段首尾高度/偏移一致，因此不会在阶段切换时瞬移；低压高度则随压力连续下降和恢复。

煤块只在“采煤机正在割煤且输送机运行”时生成；有数量上限和寿命，先受重力落入槽，再随输送机运动。刮板按 3.2 m 周期在有效槽长内做环形包裹，端点回绕藏在机头/机尾护罩内，不会在槽外多画一段。输送机停止或堵塞后不再满足联动条件。

## 状态机、暂停和急停

`SimulationController::canTransition` 是合法转换的单一入口。`transitionTo` 记录前状态、进入时间、原因并写事件日志。暂停不推进仿真时钟，Resume 回到保存的 `resumeState`。急停先把运动设备速度归零，再进入锁存状态；只有 Release 能回到 Stopped，随后才能 Reset/Initialize。

## 拾取与高亮

Camera 把鼠标 NDC 坐标乘以逆 `Projection * View` 得到世界射线。Renderer 保存本帧设备部件的 AABB，用 slab 算法求最近交点并返回设备 ID。高亮是片元着色器中的橙色基色和视角 rim 项；可选边框用线框绘制所选设备的 AABB。

## ImGui 输入隔离

Application 在处理相机键鼠前检查 `ImGuiIO::WantCaptureKeyboard/WantCaptureMouse`。光标位于滑块、列表或日志时，UI 操作不会同时转动相机或选择场景设备。
