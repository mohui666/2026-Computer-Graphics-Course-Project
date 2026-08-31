# 测试报告

测试日期：2026-08-31（Asia/Shanghai）。

## 环境

| 项目 | 实测值 |
|---|---|
| OS | Windows 10/11 系列，Windows SDK 10.0.22621.0 |
| CMake | 3.29.2 |
| 编译器 | MSVC 19.44.35223.0，Visual Studio 2022 Community |
| OpenGL | 3.3.0 NVIDIA 616.56 |
| GPU | NVIDIA GeForce RTX 4060 Laptop GPU/PCIe/SSE2 |
| 依赖 | GLM 1.0.1、GLFW 3.4、GLAD 2.0.8、ImGui 1.91.8 |

## 已执行命令与结果

### 逻辑内核首次验证

```powershell
cmake -S . -B build-core -G "Visual Studio 17 2022" -A x64 -DMINE_BUILD_APP=OFF -DMINE_BUILD_TESTS=ON
cmake --build build-core --config Debug --parallel
ctest --test-dir build-core -C Debug --output-on-failure
```

- 配置退出码：0；
- 构建退出码：0；
- CTest：1/1 test passed，0 failed；
- 测试程序内部：30/30 checks passed。

### 完整 Debug 构建

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DMINE_BUILD_APP=ON -DMINE_BUILD_TESTS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

- 最终配置退出码：0；
- 最终构建退出码：0；
- 生成 `build/bin/Debug/MineLongwallSimulation.exe`；
- CTest：1/1 passed，0 failed。

### 图形运行与帧缓冲截图

```powershell
.\build\bin\Debug\MineLongwallSimulation.exe --capture
```

- 退出码：0；
- 实测创建 OpenGL 3.3 Core 上下文；
- 实测 Renderer：NVIDIA RTX 4060 Laptop GPU；
- 自动完成初始化、输送机启动和割煤，达到至少 12 个煤块后保存 `screenshots/latest.bmp`；
- 画面检查确认：位移顶/底板和巷帮、30 台分层梯形盾板支架、分段刮板输送机、螺旋双滚筒采煤机、灯具、通风管、程序化煤岩/磨损材质、雾和 ImGui 均可见；
- 真实运行验证 RGBA16F HDR、4× MSAA resolve、Bloom、SSAO、ACES、FXAA 和最终默认后缓冲截图链路；
- 初始图形运行中发现并修复中文路径、采煤机/输送机空间错位、支架遮挡、相机与 UI 构图问题。
- 入口端专项截图确认刮板全部收束在机头/机尾护罩之间；采煤机近景确认滚筒本体、顶板和输送机保持间隙；支架完整动作周期以连续位姿回归验证。

## 自动测试覆盖

| 需求 | 测试证据 |
|---|---|
| Transform 父子矩阵 | 父子位移合成断言 |
| 状态机合法/非法转换 | Stopped→Initializing 接受、Stopped→Cutting 拒绝 |
| 采煤机端点换向 | 位置钳制、方向改变、单次事件消费 |
| 暂停冻结 | 位置和仿真时钟均不变 |
| 重置 | 急停释放后恢复确定性初始位置和 Stopped |
| 急停 | 采煤机位置冻结、输送机速度立即为 0、锁存阻止 Reset |
| 支架动作 | Waiting→Lowering→Advancing→Raising→Normal |
| 故障与清除 | ConveyorJam 进入 Fault，清除后安全回到 Stopped |
| 配置边界 | 支架数、面长、速度、煤块上限钳制 |

## 手工/视觉检查范围

已验证真实窗口可创建、着色器可编译、渲染循环运行、内部截图有效、正常 `--capture` 退出和 OpenGL/ImGui/GLFW 清理。相机/窗口缩放逻辑从代码路径与真实 framebuffer 尺寸确认。

尚未用自动 UI 脚本逐一点击所有 ImGui 按钮；故障、暂停、急停和恢复的底层行为由逻辑测试覆盖，UI 按钮调用同一公开 Controller 方法。多显示器、Intel/AMD GPU、Linux/macOS 和长时间压力运行未验证。

## Release 最终结果

```powershell
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\bin\Release\mine_logic_tests.exe
.\build\bin\Release\MineLongwallSimulation.exe --capture
```

- Release 构建退出码：0；
- Release CTest：1/1 passed，0 failed；
- 逻辑测试：30/30 checks passed；
- Release 图形冒烟：退出码 0，OpenGL 3.3 / NVIDIA RTX 4060，自动截图时状态为 `Cutting forward`、采煤机速度 5.00 m/s、煤块数 12。
- 六个 Release 固定机位均生成本轮新截图；1500×900 下约 1999～2001 个场景 Draw Calls，截图读数为 163.6～167.4 FPS。
