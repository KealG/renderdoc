# Phase 2 改动总结与 Phase 3 方案

> 生成时间：2026-03-08  
> 仓库：`/mnt/e/Project/Git/renderdoc`  
> 当前分支：`K`

## 1. 背景

本仓库当前处于一轮长期维护导向的品牌化预改造过程中。

本轮工作的总体原则始终保持不变：

- 小步修改；
- 默认行为不变；
- 默认输出不变；
- 优先降低维护成本；
- 每一步都尽量可单独回归；
- 当前阶段仍不直接改成 `ripperK` 对外输出。

在此基础上，`Phase 1` 已先完成统一品牌配置中心与工具链整理；`Phase 2` 则聚焦于“构建系统 / 工程文件 / 产物命名 / 安装器侧标识”的抽象和集中化。

---

## 2. Phase 2 范围与边界

### 2.1 目标

`Phase 2` 的核心目标是：

**先把“编译出来的东西叫什么、工程默认产物叫什么、安装器默认显示什么”这类构建期和工程期信息收敛成统一变量源。**

### 2.2 本阶段坚持的边界

本阶段明确**不进入**以下高风险区域：

- 运行时查找链路；
- 进程创建与注入链；
- self-exclusion / 防自注入逻辑；
- shim 共享内存 / 映射名 / helper 运行时路径；
- API 导出名、协议标识、文件格式、layer 导出函数。

换句话说，`Phase 2` 是“构建与外显命名抽象”，不是“运行时命名切换”。

---

## 3. Phase 2 之前的基础状态

`Phase 2` 开始前，仓库里已经具备两项基础工作：

1. `92cc4437c` — `Centralize brand identifier configuration`
   - 新增 `renderdoc/common/brand_config.h`；
   - 把一批运行时命名、路径、共享内存、崩溃处理、UI 文案、更新目录常量改为从统一配置取值；
   - 默认值仍保持 RenderDoc 原始名称。
2. `3b387e142` — `Upgrade VS project files to v143`
   - 一批 Windows `.vcxproj` 整理到 `v143`；
   - 补充 `Windows SDK 10.0`；
   - 仅为工程层工具链升级。

这两项工作为 `Phase 2` 提供了统一变量入口和稳定的 Windows 构建基础。

---

## 4. Phase 2 提交清单

本阶段共新增 8 个小步提交：

1. `80874e137` — `Abstract Windows branding target names`
2. `9a4e229f1` — `Abstract CMake output naming defaults`
3. `fee4f7b55` — `Abstract Windows version resource branding`
4. `171a5ea55` — `Abstract qrenderdoc qmake target name`
5. `9db61a99b` — `Abstract pyrenderdoc output names`
6. `252ad9abc` — `Abstract macOS bundle naming defaults`
7. `a5a728c6b` — `Abstract installer binary naming`
8. `a46981f66` — `Abstract installer identity defaults`

这些提交都遵循同一原则：

- 只做命名来源抽象；
- 默认值继续保持 RenderDoc；
- 不把运行时链路一起卷进去；
- 每一步尽量保持可独立回归。

---

## 5. Phase 2 实际改动归纳

### 5.1 Windows / MSBuild 产物命名抽象

已把 Windows 主产物与相关 target name 的默认值收敛到统一来源，减少以后多处同步修改的成本。

代表文件：

- `util/Branding.props`
- `renderdoc/common/brand_config.h`

本阶段完成后，Windows 下下列产物默认名已可由集中变量源派生：

- `renderdoc.dll`
- `qrenderdoc.exe`
- `renderdoccmd.exe`
- `renderdocshim32.dll`
- `renderdocshim64.dll`
- Python 相关模块输出名

### 5.2 CMake 默认输出名抽象

顶层 `CMakeLists.txt` 中已新增或整理一批默认输出名变量，避免后续跨平台继续散落硬编码。

代表项包括：

- `RDOC_CORE_OUTPUT_NAME`
- `RDOC_GUI_TARGET_NAME`
- `RDOC_CMD_OUTPUT_NAME`
- `RDOC_PY_CORE_MODULE_NAME`
- `RDOC_PY_GUI_MODULE_NAME`
- `RDOC_GUI_BUNDLE_ICON_NAME`
- `RDOC_CORE_MAC_DYLIB_NAME`
- `RDOC_ANDROID_OUTPUT_NAME`

代表文件：

- `CMakeLists.txt`
- `qrenderdoc/CMakeLists.txt`

### 5.3 Windows 资源信息抽象

Windows 资源文件中的品牌字符串已改为从统一品牌配置派生，覆盖了较关键的 `ProductName / InternalName / OriginalFilename` 等字段。

代表文件：

- `renderdoc/data/renderdoc.rc`
- `qrenderdoc/Resources/qrenderdoc.rc`
- `renderdoccmd/renderdoccmd.rc`

这一步的价值在于：

- 保证“文件资源显示名”和“构建输出名”后续更容易同步；
- 降低只改文件名不改版本资源而导致的不一致风险。

### 5.4 qmake / qrenderdoc 工程名抽象

`qrenderdoc` 的 qmake target、依赖链接名、macOS bundle 拷贝名等默认值已改为从变量派生。

代表文件：

- `qrenderdoc/qrenderdoc.pro`

这一步使后续如果要做 GUI 名称切换，不必继续在 qmake 文件中人工逐项改硬编码。

### 5.5 pyrenderdoc 输出名抽象

Python 模块输出名已统一改为从构建变量派生，覆盖 CMake 和 Windows 工程文件两侧。

代表文件：

- `qrenderdoc/Code/pyrenderdoc/CMakeLists.txt`
- `qrenderdoc/Code/pyrenderdoc/pyrenderdoc_module.vcxproj`
- `qrenderdoc/Code/pyrenderdoc/qrenderdoc_module.vcxproj`

### 5.6 macOS bundle / dylib 命名抽象

macOS 相关 bundle 与动态库默认名也已经做了构建期收敛。

代表文件：

- `CMakeLists.txt`
- `qrenderdoc/CMakeLists.txt`
- `qrenderdoc/qrenderdoc.pro`

虽然当前主要验证环境是 Windows，但这一步提前把跨平台命名源统一了。

### 5.7 安装器二进制名与身份信息抽象

安装器部分分两步完成：

#### 第一步：二进制文件名抽象

已把安装器中引用的主桌面端产物名集中到：

- `util/installer/Branding.wxi`

覆盖了：

- GUI EXE 名；
- Core DLL 名；
- Vulkan JSON manifest 名；
- CLI EXE 名；
- app header 名；
- shim32 / shim64 DLL 名。

#### 第二步：安装器身份信息抽象

继续把以下“安装器可见身份信息”收拢到统一变量：

- 产品显示名；
- 安装器描述；
- 帮助链接；
- 安装目录名；
- 开始菜单快捷方式名；
- 文档快捷方式名；
- `.rdc` / `.cap` 的 ProgId；
- 文件关联描述文本；
- thumbnail handler 名；
- Android APK 默认文件名；
- 64 位安装器里附带的 x86 `renderdoc.json` 名。

代表文件：

- `util/installer/Branding.wxi`
- `util/installer/Installer64.wxs`
- `util/installer/Installer32.wxs`

注意：

- 这里只是把“默认值的来源”集中化；
- 默认值仍然全部保持 `RenderDoc` 体系；
- `customtext.wxl` 的欢迎页文案本轮没有继续扩大修改，避免在当前缺少 WiX 验证环境下增加不必要风险。

---

## 6. Phase 2 的总体价值

完成 `Phase 2` 后，仓库已经具备以下条件：

1. **构建输出名不再高度散落。**
2. **Windows / CMake / qmake / macOS / Python / installer 的命名变量源明显更集中。**
3. **后续真正切换品牌时，更多工作将变成“改默认值”和“补局部逻辑”，而不是继续满仓库找硬编码。**
4. **阶段边界更清楚：构建期抽象与运行时注入链改造被明确拆开。**

从维护角度看，`Phase 2` 最重要的意义不是“改名已经完成”，而是：

**后续真正的重命名工作已经有了更稳定的支点。**

---

## 7. Phase 2 当前验证结论

### 7.1 已完成验证

已知验证结果如下：

- Windows `Development|x64` 可构建；
- Windows `Release|x64` 可构建；
- UI 可正常启动；
- 可正常打开 capture 文件；
- 设置 / 关于 页面可打开；
- 至少一组 64 位目标程序的 `Launch + Inject` 已成功；
- Windows 资源默认值在修改后保持原始 RenderDoc 输出；
- 安装器相关改动已完成静态检查与 XML 可解析性检查。

### 7.2 尚未完成验证

由于本地缺少 WiX 工具链，以下验证本轮未完成：

- `Installer32.wxs` / `Installer64.wxs` 的实际构建；
- 安装器安装后的文件关联与 ARP 展示的实际端到端验收。

因此可以把当前结论表述为：

> `Phase 2` 的代码整理工作已经基本完成；安装器部分的“静态整理”已完成，但 installer 实构建验证仍待具备 WiX 环境后补验。

---

## 8. 对 Phase 2 的边界结论

截至当前，`Phase 2` 可以视为：

- **代码层面已完成；**
- **验证层面除 installer 实构建外已基本闭环；**
- **不建议继续在 `Phase 2` 内追加运行时查找 / 注入链修改。**

后续如果继续推进，应进入 `Phase 3`。

---

## 9. Phase 3 目标

根据既有规划，`Phase 3` 的目标不是“正式改成 ripperK 对外显示”，而是：

**把运行时注入链路上仍然依赖旧硬编码名字的部分，继续改造成可配置、可集中管理、默认行为不变的结构。**

也就是说，`Phase 3` 仍建议优先做“抽象层整理”，而不是一口气做最终品牌切换。

---

## 10. Phase 3 建议拆分方案

建议把 `Phase 3` 拆成 **5 个小步**，继续保持每一步都尽量少改文件、可回归。

### Step 1：收敛运行时可执行文件 / DLL 查找名

#### 目标

把运行时对以下名字的查找逻辑收拢到统一配置入口，而不是继续直接写死：

- `qrenderdoc.exe`
- `renderdoccmd.exe`
- `renderdoc.dll`
- `renderdocshim32.dll`
- `renderdocshim64.dll`

#### 优先文件

- `renderdoccmd/renderdoccmd_win32.cpp`
- `qrenderdoc/renderdocui_stub.cpp`
- 必要时补充 `renderdoc/common/brand_config.h`

#### 改造原则

- 只收拢名字来源；
- 不改路径结构；
- 不改查找顺序；
- 不改默认返回值；
- 默认仍指向现有 RenderDoc 产物名。

#### 建议回归

- `Development|x64` 构建；
- 启动 UI；
- 打开已有 capture；
- `Launch + Inject` 一个已知可工作的 64 位程序。

### Step 2：收敛 child-process 注入的自排除名单

#### 目标

把注入链上防自递归、自排除逻辑中的旧名字常量改为统一配置来源。

#### 优先文件

- `renderdoc/os/win32/sys_win32_hooks.cpp`
- `renderdoc/os/win32/win32_process.cpp`
- 必要时补充公共 helper

#### 改造原则

- 只抽象“比较用的名称”；
- 不改变判断时机；
- 不改变过滤逻辑；
- 不把 shim / helper / UI / cmd 的行为一起改乱。

#### 建议回归

- `Launch + Inject` 已知可工作程序；
- 注入已有进程；
- 确认不会出现自注入递归；
- 确认不会误把目标程序过滤掉。

### Step 3：收敛 shim 共享对象名与辅助通信标识

#### 目标

把 shim 侧使用的共享内存名、映射名、辅助 DLL 名等集中到统一配置来源。

#### 优先文件

- `renderdocshim/renderdocshim.h`
- `renderdoc/os/win32/win32_process.cpp`
- 其他直接引用 shim 标识的少量文件

#### 改造原则

- 继续保持默认名字不变；
- 只抽象命名来源；
- 不在这一小步里同时修改注入流程控制逻辑。

#### 建议回归

- 构建 `Development|x64`；
- `Launch + Inject`；
- 如果你有可用的全局 hook / system-wide 场景，再补一次轻量冒烟。

### Step 4：收敛 crash helper / update helper / stub 路径名

#### 目标

把 crash helper、update helper、UI stub 等辅助路径构造里的旧名硬编码统一到配置层。

#### 优先文件

- `renderdoc/core/crash_handler.h`
- `qrenderdoc/renderdocui_stub.cpp`
- 与 update / crash handle 路径构造直接相关的少量文件

#### 改造原则

- 优先只改“文件名常量”和“拼路径使用点”；
- 不改 crash 处理流程；
- 不改 updater 行为顺序。

#### 建议回归

- UI 正常启动；
- 设置 / 关于 页面正常；
- 打开 capture 正常；
- 启动时无缺失 helper / stub 相关报错。

### Step 5：统一收尾并补一次 Phase 3 阶段性联调

#### 目标

把前四步涉及的运行时命名常量再做一轮统一复核，清掉明显重复项，并做一轮阶段性联调。

#### 重点内容

- 复核 `brand_config` 是否已经能覆盖运行时链核心名字；
- 检查是否还有散落的旧硬编码仅剩 1~2 处；
- 确认构建期命名源与运行时查找名没有相互脱节。

#### 建议回归

至少覆盖：

- `Development|x64` 构建；
- `Release|x64` 构建；
- UI 启动；
- 打开 capture；
- `Launch + Inject` 新进程；
- 注入已有进程；
- 检查不会因 helper 名不一致导致启动失败。

---

## 11. Phase 3 执行建议

为了继续保持低风险，建议遵守以下节奏：

1. **一次只推进一个小步。**
2. **每一步尽量控制在 1~4 个核心文件内。**
3. **先抽象名字来源，再考虑是否真的切换默认值。**
4. **优先改查找名 / 比较名 / 路径拼接名，不先碰协议导出名。**
5. **每一步都至少补一次 `Launch + Inject` 验证。**

如果后续要正式进入品牌切换阶段，建议等 `Phase 3` 结束后，再单独规划“默认值切换”和“兼容移除”工作，而不要在 `Phase 3` 中途混做。

---

## 12. 当前建议结论

当前建议是：

- 认为 `Phase 2` 的代码整理工作已经完成；
- installer 实构建验证留待有 WiX 环境时补验；
- 下一阶段从 `Phase 3 / Step 1` 开始；
- 第一个实际落点建议选：
  - `renderdoccmd/renderdoccmd_win32.cpp`
  - `qrenderdoc/renderdocui_stub.cpp`
  - `renderdoc/common/brand_config.h`

原因是这一小步最符合当前策略：

- 风险比注入自排除逻辑低；
- 价值又明显高于继续整理静态资源；
- 仍然可以保持默认行为完全不变；
- 回归手段也最成熟。

