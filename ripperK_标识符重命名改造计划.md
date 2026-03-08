# ripperK 标识符重命名改造计划

## 1. 目标

基于当前 fork 的 RenderDoc 源码，规划一套**系统性的品牌与特征码重命名方案**，把 `RenderDoc / renderdoc / RENDERDOC / qrenderdoc / renderdoccmd / renderdocshim` 相关标识，迁移为 `ripperK` 系列命名。

本计划覆盖：

- 构建系统与产物命名
- VS 解决方案与项目文件
- 核心 API 头文件与导出符号
- 进程创建、注入、hook、自排除/白名单逻辑
- 崩溃处理器、共享内存、Shim DLL、事件名、socket 标识
- 默认文件路径、注册表、Vulkan layer、Android 包名
- Qt UI 层、资源、文档、安装器、打包产物
- 输出文件名、capture 扩展名、文件头/魔数、协议/格式特征
- Python 绑定、测试、脚本、说明文档中残留的旧名称

---

## 2. 建议先明确的命名映射

为了避免后续反复返工，建议先冻结一份命名基线。

### 2.1 显示名称

- `RenderDoc` → `ripperK`
- `QRenderDoc` → `QripperK` 或直接统一显示为 `ripperK`

### 2.2 二进制/模块名

建议采用下面这套较一致的映射：

| 现名 | 建议新名 |
| --- | --- |
| `renderdoc.dll` / `librenderdoc.so` | `ripperK.dll` / `libripperK.so` |
| `qrenderdoc.exe` | `qripperK.exe` |
| `renderdoccmd.exe` | `ripperKcmd.exe` |
| `renderdocshim32.dll` | `ripperKshim32.dll` |
| `renderdocshim64.dll` | `ripperKshim64.dll` |
| `renderdoc.json` / `renderdoc_capture.json` | `ripperk.json` / `ripperk_capture.json` |
| `renderdoc_app.h` | `ripperk_app.h` |
| `renderdoc_replay.h` | `ripperk_replay.h` |
| Python `renderdoc` 模块 | `ripperk` |
| Python `qrenderdoc` 模块 | `qripperk` |

### 2.3 文件扩展名

这里建议先做决策门：

- **低风险兼容模式**：先保留 `.rdc` / `.cap`，只改产品名与产物名。
- **完全去 RenderDoc 特征模式**：连扩展名一起迁移，例如：
  - `.rdc` → `.rpk`
  - `.cap` → `.rkc` 或 `.rpkcfg`

如果你的目标是“尽量减少 RenderDoc 识别痕迹”，建议最终走第二种；但它会明显扩大兼容性改动面。

---

## 3. 我扫描到的关键耦合点

下面是当前仓库里最重要的一批命名耦合点，后续改造应围绕这些点展开。

### 3.1 构建与工程入口

- 顶层工程名：`CMakeLists.txt:201`
- 基础产物名变量：`CMakeLists.txt:112`
- Qt UI 目标名：`qrenderdoc/qrenderdoc.pro:15`
- macOS 图标/Bundle 资源名：`qrenderdoc/CMakeLists.txt:94`
- Android 包名：`renderdoccmd/CMakeLists.txt:222`
- Windows 解决方案项目名：`renderdoc.sln:8`、`renderdoc.sln:10`、`renderdoc.sln:21`、`renderdoc.sln:42`

### 3.2 核心 API 与导出符号

- App API 头：`renderdoc/api/app/renderdoc_app.h:57`
- API 特征魔数：`renderdoc/api/app/renderdoc_app.h:73`
- 导出 API 名称约束：`renderdoc/api/app/renderdoc_app.h:854`
- replay API 中的 replay marker：`renderdoc/api/replay/renderdoc_replay.h:52`
- 导出函数实现：`renderdoc/replay/app_api.cpp:404`
- 动态查找导出函数：`renderdoc/replay/entry_points.cpp:555`

### 3.3 全局配置、端口、Layer 与 Android 名称

- 目标控制端口/远程端口：`renderdoc/common/globalconfig.h:143`
- Vulkan Layer 名：`renderdoc/common/globalconfig.h:153`
- Android 动态库名：`renderdoc/common/globalconfig.h:156`
- Android 包名前缀：`renderdoc/common/globalconfig.h:159`

### 3.4 注入、Hook、自排除逻辑、Shim 与共享对象名

- Windows 子进程注入自排除：`renderdoc/os/win32/sys_win32_hooks.cpp:340`
- Shim 共享内存名与 DLL 名：`renderdocshim/renderdocshim.h:36`
- Crash handle 事件名：`renderdoc/core/crash_handler.h:131`
- Crash helper 启动的命令名：`renderdoc/core/crash_handler.h:138`
- `renderdoccmd` 内部对 `qrenderdoc.exe` / `renderdoc.dll` 的硬编码：`renderdoccmd/renderdoccmd_win32.cpp:437`、`renderdoccmd/renderdoccmd_win32.cpp:818`
- Windows 辅助 stub 中的 `qrenderdoc.exe`：`qrenderdoc/renderdocui_stub.cpp:60`

### 3.5 默认路径、AppData、Temp、日志与配置文件

- Windows AppData 子目录：`renderdoc/os/win32/win32_stringio.cpp:389`
- Windows 临时输出 `.rdc` / `.log` 路径：`renderdoc/os/win32/win32_stringio.cpp:358`
- POSIX 临时输出 `.rdc` / `.log` 路径：`renderdoc/os/posix/posix_stringio.cpp:234`
- 崩溃 dump 目录：`renderdoc/core/crash_handler.h:62`
- Qt UI 的 `AppDataLocation` 配置路径：`qrenderdoc/Code/Interface/QRDInterface.cpp:165`
- UI 默认配置/最近捕获文件：`qrenderdoc/Code/qrenderdoc.cpp:521`、`qrenderdoc/Windows/Dialogs/CaptureDialog.cpp:98`

### 3.6 Capture 格式、扩展名与文件头特征

- RDC 文件头 magic：`renderdoc/serialise/rdcfile.cpp:133`
- 临时 capture 生成路径：`qrenderdoc/Code/CaptureContext.cpp:281`
- 多处 `OpenFile(..., "rdc", ...)`：`qrenderdoc/Code/ReplayManager.cpp:454`、`qrenderdoc/Windows/MainWindow.cpp:835`
- 安装器里的 `.rdc` / `.cap` 文件关联：`util/installer/Installer64.wxs:49`、`util/installer/Installer64.wxs:66`

### 3.7 Vulkan Layer 与可见特征字符串

- Vulkan manifest 模板：`renderdoc/driver/vulkan/renderdoc.json:1`
- Layer 输出名与环境变量：`renderdoc/driver/vulkan/CMakeLists.txt:117`
- 注册表/系统 layer 注册逻辑：`renderdoc/driver/vulkan/vk_win32.cpp:359`、`renderdoc/driver/vulkan/vk_posix.cpp:356`
- Layer 导出函数名 `VK_LAYER_RENDERDOC_Capture*`：`renderdoc/driver/vulkan/vk_layer.cpp:469`
- Vulkan 内部 app/tooling 字符串：`renderdoc/driver/vulkan/wrappers/vk_device_funcs.cpp:49`

### 3.8 UI、资源、品牌字符串、更新/崩溃/网络端点

- UI 资源 ProductName：`qrenderdoc/Resources/qrenderdoc.rc:92`
- 核心 DLL 资源 ProductName：`renderdoc/data/renderdoc.rc:92`
- `renderdoc.dll` / `renderdoccmd.exe` 更新流程：`qrenderdoc/Windows/Dialogs/UpdateDialog.cpp:258`
- Bug report URL：`qrenderdoc/Code/Interface/PersistantConfig.h:159`
- Analytics 上报地址：`qrenderdoc/Code/Interface/Analytics.cpp:598`

### 3.9 安装器/注册表/文件关联

- 安装器产物名：`util/installer/Installer64.wxs:39`、`util/installer/Installer64.wxs:76`、`util/installer/Installer64.wxs:107`
- 应用关联：`util/installer/Installer64.wxs:262`
- ProgId / 文件类型描述：`util/installer/Installer64.wxs:325`
- 安装器欢迎/品牌文本：`util/installer/customtext.wxl:4`

---

## 4. 核心改造原则

### 4.1 不建议“一把梭”全局替换

原因：

- `RenderDoc`、`renderdoc`、`RENDERDOC` 分别处于**显示名、文件名、宏/API 前缀、协议特征**四种不同层面。
- 其中有些可以直接改文本；有些必须联动改调用方、manifest、注册表、脚本、测试；还有些改完会直接破坏向后兼容。

### 4.2 建议先做“品牌配置中心”再分模块替换

建议在第一阶段先建立统一命名源，而不是散落式硬编码替换。可考虑引入：

- 一个顶层 CMake 品牌变量集合
- 一个运行时品牌配置头，例如 `brand_config.h`
- 对应的资源/安装器模板变量

建议抽出的品牌常量至少包括：

- 产品显示名
- 核心库名
- UI 名
- CLI 名
- Shim 名
- Vulkan layer 名
- Vulkan 环境变量名
- Android 包名
- Capture 扩展名
- Settings 扩展名
- AppData 子目录名
- Temp 子目录名
- Crash 事件名
- 共享内存名
- 导出 API 前缀与函数名
- 网络 URL（analytics / bugreport / update）

### 4.3 明确区分“外显重命名”和“协议/格式重命名”

这两类工作必须分开推进：

- **外显重命名**：文件名、窗口名、安装器、路径、注册表、UI 文本、包名、模块名。
- **协议/格式重命名**：导出函数名、magic、capture 文件头、Vulkan layer 导出函数、socket 标识、共享内存名。

第二类风险明显更高，应晚于第一类落地。

---

## 5. 分阶段实施计划

## Phase 0：冻结命名策略与兼容边界

### 目标

先明确这次 fork 到底是：

- **品牌重制**，还是
- **品牌重制 + 全特征改名**。

### 需要做的决策

1. 是否保留旧 API 兼容入口：
   - 是否保留 `RENDERDOC_GetAPI`
   - 是否保留旧头文件别名 `renderdoc_app.h` / `renderdoc_replay.h`
2. 是否保留 `.rdc` / `.cap`
3. 是否保留旧 Vulkan Layer 导出函数名
4. 是否保留旧 Android 包名/旧 layer 名做兼容壳
5. 是否保留旧 Python 模块导入名 `renderdoc` / `qrenderdoc`

### 建议

如果目标是“定制版但仍要低成本维护”，建议：

- Phase 1 保留旧 capture 扩展名与旧 API 入口；
- Phase 2 再决定是否彻底切断兼容。

---

## Phase 1：建立品牌配置中心

### 目标

把目前散落在各处的命名常量收拢成统一配置，减少后续人工漏改。

### 计划内容

1. 在顶层 CMake 中新增品牌变量：
   - 产品名
   - 基础模块名
   - UI 名
   - CLI 名
   - Shim 名
   - Layer 名
   - Android 包名
   - 默认目录名
   - 扩展名
2. 在运行时头文件中建立统一常量入口。
3. 让 installer、rc、qmake、manifest、脚本尽量从同一个变量源派生。

### 代表性入口文件

- `CMakeLists.txt:112`
- `renderdoc/common/globalconfig.h:153`
- `qrenderdoc/qrenderdoc.pro:15`
- `renderdoc/driver/vulkan/CMakeLists.txt:117`

### 完成标准

- 后续 80% 以上命名修改可通过配置变量驱动，而不是继续硬编码散改。

---

## Phase 2：构建系统、工程文件、产物名统一改名

### 目标

先把“编译出来的东西叫什么”统一起来。

### 计划内容

1. 重命名顶层工程与 VS solution/project：
   - `renderdoc.sln`
   - `renderdoc/*.vcxproj`
   - `renderdoccmd/*.vcxproj`
   - `renderdocshim/*.vcxproj`
   - `qrenderdoc/*.vcxproj`
   - `qrenderdoc/Code/pyrenderdoc/*.vcxproj`
2. 修改 qmake target、Windows 资源、macOS icon/bundle 名。
3. 修改生成物输出名：
   - DLL / SO / EXE / APK / CHM / QHelp / JSON manifest
4. 修改自更新流程里对新产物名的复制/查找逻辑。

### 代表性文件

- `renderdoc.sln:8`
- `qrenderdoc/qrenderdoc.pro:15`
- `qrenderdoc/Resources/qrenderdoc.rc:91`
- `renderdoc/data/renderdoc.rc:91`
- `renderdoccmd/renderdoccmd.rc:102`
- `qrenderdoc/Windows/Dialogs/UpdateDialog.cpp:258`

### 风险

- Windows 资源字符串与输出文件名不同步，导致安装器或 updater 失效。
- VS 工程引用链断裂。

---

## Phase 3：进程创建、注入、hook、自排除与 shim 改名

### 目标

把运行时注入链路上的“硬编码旧名字”全部换掉。

### 计划内容

1. 改 `renderdoccmd.exe`、`qrenderdoc.exe`、`renderdoc.dll`、`renderdocshimXX.dll` 的查找逻辑。
2. 修改 child-process 注入时的自排除逻辑，避免继续以旧名做过滤。
3. 修改 shim 共享内存名、映射名、DLL 名。
4. 修改 crash helper 的事件名与启动命令名。
5. 修改辅助 stub 和 update / crash handle / global hook 路径构造。

### 代表性文件

- `renderdoc/os/win32/sys_win32_hooks.cpp:340`
- `renderdoc/os/win32/win32_process.cpp:1500`
- `renderdocshim/renderdocshim.h:36`
- `renderdoc/core/crash_handler.h:131`
- `renderdoccmd/renderdoccmd_win32.cpp:437`
- `qrenderdoc/renderdocui_stub.cpp:60`

### 额外说明

你提到的“进程注入白名单处”，在当前代码里更准确地说是：

- **注入自排除/防自递归名单**
- **shim / crash / helper 进程的硬编码文件名**

这些都必须一起改，否则改一半会造成自注入循环或找不到辅助程序。

---

## Phase 4：核心 API、头文件、导出函数、协议标识改名

### 目标

把最核心的一层“RenderDoc API 特征”迁移到 `ripperK`。

### 计划内容

1. 头文件命名迁移：
   - `renderdoc_app.h` → `ripperk_app.h`
   - `renderdoc_replay.h` → `ripperk_replay.h`
2. API 前缀迁移：
   - `RENDERDOC_*` → `RIPPERK_*`
   - `pRENDERDOC_*` → `pRIPPERK_*`
3. 导出函数迁移：
   - `RENDERDOC_GetAPI` → `RIPPERK_GetAPI`
4. replay marker 迁移：
   - `renderdoc__replay__marker` → `ripperk__replay__marker`
5. 文档、示例、动态加载代码中的导出名同步更新。

### 代表性文件

- `renderdoc/api/app/renderdoc_app.h:854`
- `renderdoc/api/replay/renderdoc_replay.h:52`
- `renderdoc/replay/app_api.cpp:404`
- `renderdoc/replay/entry_points.cpp:555`

### 风险

这是**兼容性风险最高**的一步：

- 所有使用旧 header 的外部项目会直接失效；
- 所有用 `GetProcAddress("RENDERDOC_GetAPI")` 的调用者都会失效；
- Python / 文档 / 示例 / 测试需要同步迁移。

### 建议

可以采用两阶段：

- 先新增 `RIPPERK_*`，保留旧入口别名；
- 验证稳定后再移除旧入口。

如果你的目标是“完全去旧特征”，则第二阶段必须最终清理掉旧别名。

---

## Phase 5：Vulkan Layer、Android 包名与工具识别字符串改名

### 目标

清理 Vulkan / Android 体系下最显眼的一批可识别特征。

### 计划内容

1. 重命名 Vulkan manifest 文件、layer 名、enable/disable 环境变量。
2. 重命名 `VK_LAYER_RENDERDOC_Capture*` 一系列导出函数。
3. 修改 Windows/Posix 的 layer 注册与 JSON 路径逻辑。
4. 修改 Android library name 与 package name。
5. 修改 Vulkan 内部 `VkApplicationInfo` 和 tooling info 的字符串。

### 代表性文件

- `renderdoc/common/globalconfig.h:153`
- `renderdoc/common/globalconfig.h:156`
- `renderdoc/common/globalconfig.h:159`
- `renderdoc/driver/vulkan/renderdoc.json:1`
- `renderdoc/driver/vulkan/CMakeLists.txt:117`
- `renderdoc/driver/vulkan/vk_layer.cpp:469`
- `renderdoc/driver/vulkan/wrappers/vk_device_funcs.cpp:49`

### 风险

- Vulkan loader 找不到 layer
- 注册表/implicit_layer.d 仍指向旧 manifest
- Android 端 package / so / loader 名不同步

---

## Phase 6：默认路径、注册表、安装器、文件关联改名

### 目标

把用户落盘可见路径与系统注册信息中的旧品牌全部迁移掉。

### 计划内容

1. 修改 AppData / Temp / crash dump / log / config 目录名。
2. 修改默认输出文件名模板与 capture/log 文件名。
3. 修改安装器中的：
   - ProductName
   - Manufacturer
   - 文件名
   - 文件关联描述
   - ProgId
   - OpenWith 注册表项
   - Vulkan layer 注册项
4. 修改 CHM/QHelp/doc 产物名。

### 代表性文件

- `renderdoc/os/win32/win32_stringio.cpp:358`
- `renderdoc/os/posix/posix_stringio.cpp:234`
- `renderdoc/core/crash_handler.h:62`
- `util/installer/Installer64.wxs:39`
- `util/installer/Installer64.wxs:262`
- `util/installer/Installer64.wxs:325`
- `util/installer/customtext.wxl:4`

### 注意

如果你决定连 `.rdc` / `.cap` 扩展名都换掉，这一阶段需要同步修改：

- 安装器文件关联
- 文件选择器过滤器
- `OpenFile(..., "rdc", ...)`
- capture format 列表
- Shell/OpenWith 注册表
- Linux desktop mime 信息

---

## Phase 7：capture 扩展名、文件头 magic、协议特征码改名

### 目标

清理 `.rdc` 与 RDC 文件头等格式级识别特征。

### 计划内容

1. 修改 capture 扩展名与 settings 扩展名。
2. 修改文件头 magic：
   - `renderdoc/serialise/rdcfile.cpp:133`
3. 修改 rewrite / recompress / temp capture 路径。
4. 修改所有 `OpenFile(..., "rdc", ...)` 和 UI 文件过滤器。
5. 修改上传 crash report 时带的文件名。
6. 检查 remote copy / recompress / export/import 对临时后缀的假设。

### 代表性文件

- `renderdoc/serialise/rdcfile.cpp:133`
- `renderdoc/core/core.cpp:1698`
- `qrenderdoc/Code/CaptureContext.cpp:281`
- `qrenderdoc/Windows/MainWindow.cpp:601`
- `qrenderdoc/Widgets/ReplayOptionsSelector.cpp:204`
- `qrenderdoc/Windows/Dialogs/CrashDialog.cpp:435`

### 风险

这是**格式兼容性最高风险**的一步：

- 旧版 RenderDoc 将无法直接打开新 capture；
- 如果文件头 magic 一起换，现有工具链/脚本会全部失效；
- 如果只改扩展名不改文件头，会留下格式级旧特征；
- 如果只改文件头不改解析逻辑，会直接无法读取。

### 建议

先实现“双读单写”过渡：

- 可以读旧 `RDOC` 和新 magic
- 只写新 magic

这样方便迁移与回归测试。

---

## Phase 8：Qt UI、Python 绑定、脚本、测试、文档收尾

### 目标

清掉最后一层人类可见和脚本可见的旧品牌残留。

### 计划内容

1. 改 UI 显示文本、About、CrashDialog、Analytics、帮助菜单、更新说明。
2. 改 Python 模块名、stub 生成脚本、示例导入语句。
3. 改测试脚本里对 `renderdoc` 模块、`renderdoc.dll`、`libVkLayer_GLES_RenderDoc.so` 的硬编码。
4. 改 docs、README、URL、GitHub 链接、bug report/analytics/update endpoint。
5. 决定是否保留原 upstream 联系方式与说明文本。

### 代表性文件

- `qrenderdoc/Resources/qrenderdoc.rc:92`
- `qrenderdoc/Code/Interface/PersistantConfig.h:159`
- `qrenderdoc/Code/Interface/Analytics.cpp:598`
- `docs/in_application_api.rst:14`
- `docs/regenerate_stubs.py:42`
- `util/test/run_tests.py:8`
- `util/test/demos/test_common.cpp:631`

### 额外建议

如果这是一个长期维护 fork，建议在这一阶段统一替换：

- 文档名
- QHelp / CHM 文件名
- 官网/更新/符号服务器 URL
- 社区扩展仓库链接
- telemetry / bugreport 域名

否则最终还是会留下明显的 upstream 品牌痕迹。

---

## 6. 建议的优先级顺序

推荐按下面顺序落地，而不是并行全改：

1. **品牌配置中心**
2. **构建系统与产物名**
3. **注入 / shim / crash / helper 名称**
4. **Vulkan / Android 名称**
5. **路径 / 注册表 / 安装器 / 文件关联**
6. **Qt UI / Python / 文档 / 测试**
7. **最后再改 API 导出和 capture 格式 magic**

原因是：

- 前 1~5 步主要影响构建与运行路径；
- 第 6 步主要影响人机界面和生态；
- 第 7 步最容易一次性打断兼容与回归，因此必须压后。

---

## 7. 验证计划

每完成一个阶段，建议执行对应验证。

### 7.1 构建验证

- CMake Debug/Release 可通过
- VS 解决方案可通过
- Windows / Linux 至少各完成一次本地构建
- Android 包名/so 名/manifest 输出正确

### 7.2 运行验证

- `qripperK.exe` 可启动
- `ripperKcmd.exe` 可执行 `version` / `help`
- 注入已有进程成功
- 启动并注入新进程成功
- crash helper 能被正确拉起
- global hook 不会自注入递归

### 7.3 功能验证

- 本地 capture 成功
- 打开 capture 成功
- replay / event browser / texture viewer 正常
- Vulkan layer 可注册、可捕获
- Android 端可部署、可连接、可抓帧

### 7.4 兼容性验证

如果你改了 API/格式：

- 旧 capture 是否还能读
- 新 capture 是否只能由新 fork 读
- Python 新模块名是否可导入
- 外部项目是否还能通过旧 API 头接入

---

## 8. 风险清单

### 高风险

- 改 `RENDERDOC_GetAPI` / `RENDERDOC_*` 前缀
- 改 replay marker
- 改 `.rdc` / 文件头 magic
- 改 Vulkan layer 导出函数名
- 改 Android 包名 + so 名 + loader 联动

### 中风险

- 改 `renderdoccmd.exe` / `qrenderdoc.exe` / `renderdoc.dll` 查找逻辑
- 改 shim 名、共享内存名、crash event 名
- 改安装器、注册表、文件关联
- 改 bugreport/analytics/update URL

### 低风险

- 改 UI 文本、资源 ProductName、About 文案
- 改 README / docs / 注释 / 示例
- 改 solution/project 显示名

---

## 9. 推荐的实际实施方式

为了让这次改造可维护，推荐下面的实现策略：

### 方案 A：一次性全量替换

适合：

- 你明确不需要和原版 RenderDoc 兼容
- 你可以接受较长时间的回归修复

优点：

- 最终特征最干净

缺点：

- 前期非常容易崩
- 定位问题成本高

### 方案 B：双轨兼容迁移（推荐）

步骤：

1. 先新增 `ripperK` 命名体系；
2. 临时保留旧入口和旧格式读取能力；
3. 跑完整回归；
4. 再逐步移除旧兼容壳。

优点：

- 更稳
- 便于定位哪一层改坏了
- 更适合长期维护 fork

---

## 10. 我建议的首批落地清单

如果要开始真正动手，我建议第一批先改这些文件：

1. `CMakeLists.txt`
2. `renderdoc/common/globalconfig.h`
3. `qrenderdoc/qrenderdoc.pro`
4. `qrenderdoc/Resources/qrenderdoc.rc`
5. `renderdoc/data/renderdoc.rc`
6. `renderdoccmd/renderdoccmd.rc`
7. `renderdocshim/renderdocshim.h`
8. `renderdoc/os/win32/sys_win32_hooks.cpp`
9. `renderdoc/os/win32/win32_process.cpp`
10. `renderdoc/core/crash_handler.h`
11. `renderdoc/driver/vulkan/CMakeLists.txt`
12. `renderdoc/driver/vulkan/renderdoc.json`
13. `util/installer/Installer32.wxs`
14. `util/installer/Installer64.wxs`
15. `util/installer/customtext.wxl`

这批做完，基本就能先把：

- 构建名
- 产物名
- 路径名
- 注入链路辅助名
- layer/installer 外显特征

先统一到位。

---

## 11. 结论

这次改造不是单纯的字符串替换，而是一个跨越：

- 构建系统
- 二进制产物
- 运行时注入链
- Vulkan/Android 平台接入
- capture 格式
- UI 与生态脚本

的**全链路品牌与协议重命名工程**。

最关键的建议只有两条：

1. **先做品牌配置中心，再做大面积替换**；
2. **把“外显重命名”和“协议/格式重命名”拆成两阶段执行**。

这样改造成本和回归风险会低很多，也更适合你后续继续维护这个 fork。
