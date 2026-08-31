# 重构说明

## 起点

任务开始时仓库为空，没有可扩展的原型。这里的“重构”指开发过程中主动避免典型单文件 OpenGL 原型，并在真实构建/运行反馈后修正设计。

## 原型风险与处理

| 原型问题 | 后果 | 重构/设计 | 主要文件 | 收益 |
|---|---|---|---|---|
| 窗口、业务、渲染塞入 `main.cpp` | 难测试、生命周期混乱 | Application、Renderer、Simulation、UI 分层 | `src/core`、`graphics`、`simulation`、`ui` | 逻辑测试无需窗口 |
| 每个设备各建缓冲区 | 重复显存与释放风险 | 共享 Cube/Cylinder/Rock Mesh，多 Model 矩阵 | `Mesh.*`、`Renderer.cpp` | 资源复用、集中 RAII |
| 每帧随机设备参数 | 遥测跳变、无法复现 | 连续函数与 approach 收敛 | `Shearer.cpp` | 演示和测试确定 |
| UI 直接强制设备状态 | 可绕过联锁 | 所有流程进入 Controller 状态机 | `SimulationController.*` | 故障/急停规则一致 |
| 支架统一动画 | 不符合跟机移架 | 独立 stage、timer、trigger | `HydraulicSupport.*` | 顺序可见且可测 |
| 支架阶段切换重置偏移 | 移架结束后瞬移且行程可能侵入输送机 | 连续 Smoothstep 位姿与 0.18 m 安全行程 | `HydraulicSupport.*`、`Renderer.cpp` | 阶段边界连续并保留设备间隙 |
| 刮板只包裹正端 | 机尾外出现多余刮板 | 固定间距环形位置并限制在有效槽长 | `ScraperConveyor.*` | 机头机尾外无孤立构件 |
| Windows 窄字符资源路径 | 中文目录下 Shader 打不开 | UTF-8 `filesystem::u8path` | `Shader.cpp` | 中文路径真实运行 |
| GLAD 生成器隐含手工依赖 | 新环境构建失败 | CMake 检查并从固定源码安装 | `CMakeLists.txt` | 配置可复现 |
| 默认相机在巷帮外 | 程序运行但场景被遮挡 | 固定视角移入巷道，顶视剖面 | `Camera.cpp`、`Renderer.cpp` | 启动画面可辨认 |
| 图形捕获工具无法可靠捕获 OpenGL | 无可复核画面证据 | F12 / `--capture` 直接读帧缓冲 | `Renderer.cpp`、`Application.cpp` | 独立于桌面捕获 |
| 默认后缓冲直接输出且逐片元提前 tone map | 高光受限、无法 Bloom、锯齿明显 | 4× HDR MSAA → resolve → Bloom/SSAO → ACES/FXAA | `PostProcessor.*`、`assets/shaders` | 线性高动态范围与稳定后处理 |
| 煤壁/顶底板靠大盒与重复小砖 | 轮廓平直、人工网格感强 | 一次上传的确定性位移网格与重算法线 | `Mesh.*`、`Renderer.cpp` | 更自然且只增加少量 Draw Call |
| 支架后护板是连续大青墙 | 遮住机构、重复感强 | 梯形暗钢外壳、内嵌板、加强筋和销钉跟随同一连续位姿 | `Mesh.*`、`Renderer.cpp` | 结构可读且动画不漂移 |

## 所有权设计

OpenGL 资源类不可复制，移动时用 `std::exchange` 交接句柄。Renderer 由 Application 的 `unique_ptr` 持有，确保在 GLFW 窗口销毁前显式析构。Equipment 同样禁止复制，支架容器预留容量后原地构造。

## 剩余限制

- Renderer 的场景组合仍在一个实现文件中；若继续加入转载机、皮带机和更多巷道，应拆成 `EnvironmentRenderer` 与各设备 Visual；
- AABB 使用部件轴对齐近似，旋转截齿的拾取范围略大；
- 材质是颜色参数而非图片纹理，UV 已存在但未引入纹理缓存；
- 多个支架仍是逐对象绘制，下一步可把静态/相同阶段部件批处理或实例化；
- SSAO 只提供局部接触遮蔽，尚无直接光源 Shadow Map；发光灯通过直接光源、emissive 与 Bloom 表达。
