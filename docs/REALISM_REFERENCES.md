# 现实与开源参考

本次保留 C++17 / OpenGL 3.3 Core，参考资料用于校正设备结构和渲染方法。没有把厂商照片作为纹理打包，也没有引入第三方模型或复制整个开源渲染器。

## 实景与设备

| 来源 | 观察与本项目对应修改 |
| --- | --- |
| [FUCHS — Mining Underground Coal](https://www.fuchs.com/au/en/industries/f-m/mining-underground-coal/) | 实际查看页面关联的综采照片：煤壁在左、倾斜立柱和顶梁在右、输送机居中；用窄视廊、裸露活塞杆、支架下灯具与弯曲管线重建空间关系。 |
| [Victaulic — Cutting for Coal](https://www.victaulic.com/blog/cutting-for-coal-the-long-and-short-of-longwall-mining/) | 实际查看带设备标注的井下照片：滚筒轴向煤壁、采煤机贴近 AFC；支架后方改为碎岩采空区，端头保留巷道支护。 |
| [HBT — Longwall Shearers 产品册](https://hbt-group.com/wp-content/uploads/2023/10/HBT-Longwall-Shearers_Brochure.pdf) | 主机架、摇臂、捕获式滑靴及模块化驱动说明：保留低机身结构，补充管路、紧固件、喷嘴、连续调高与机头电机。 |

## 开源渲染参考

- [JoeyDeVries / LearnOpenGL](https://github.com/JoeyDeVries/LearnOpenGL)：结合其 [PBR Lighting](https://learnopengl.com/PBR/Lighting) 讲解，实现 GGX 分布、Smith 遮蔽、Schlick 菲涅耳与能量分配。已有 HDR / ACES 管线继续使用。
- [LearnOpenGL Shadow Mapping 示例](https://github.com/JoeyDeVries/LearnOpenGL/tree/master/src/5.advanced_lighting/3.1.3.shadow_mapping)：参考深度比较、斜率偏移及 PCF 方法。此项目改用两个 2048×2048 深度数组层，服务于附近工作灯的向下光锥。
- [azer89 / SimpleOpenGL](https://github.com/azer89/SimpleOpenGL)：参考其 PBR、SSAO、Shadow Mapping 和 Bloom 的组合方式；保持本项目自己的前向渲染架构，没有迁入其延迟管线、模型或 IBL。

## 实现边界

- 所有截图来自本机实际 OpenGL 渲染，模型、煤岩表面、污渍和链环均为程序生成。
- 灯光由两盏带阴影的近灯和其余补光灯组成；不是全局光照或全光源阴影。喷雾是视觉粒子，不是流体或粉尘浓度计算。
- 顶视图自动隐藏顶板和后方碎岩，渲染面板也提供手动剖切。剖切同时开启检查补光，只改变显示，不改变仿真状态；正常井下视角不开启此补光。
- 本项目没有按某个真实设备型号逐尺寸复刻；工作面没有持续体积挖掘、真实矿压或永久累积移架。设备速度保留课堂演示尺度。
