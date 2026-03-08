# Phase 1~8 品牌抽象完成情况与自定义品牌操作指南

- 分支：`K`
- 基线提交：`50bbdd433`（`Centralize Phase 4 compatibility branding`）
- 更新时间：`2026-03-08`
- 主要来源：
  - `ripperK_标识符重命名改造计划.md`
  - `Phase2_总结与Phase3方案.md`
  - `RenderDoc_仓库代码工作流程与功能逻辑说明.md`
  - `92cc4437c` ~ `50bbdd433` 之间的实际提交历史

## 1. 当前总体结论

当前仓库已经完成了一轮以“**品牌抽象优先、默认行为不变、默认输出不变**”为原则的 `Phase 1~8` 收口。

这轮工作的核心价值不是立即把工程切成 `ripperK`，而是先把原来散落在各处的：

- 可执行文件名
- DLL / SO 名
- helper / stub / shim / crash 相关路径与命令名
- UI 显示文案
- Python 模块名
- 文档 URL / GitHub URL / 邮件地址
- capture 扩展名 / 文件类型 / magic
- Vulkan / Android 相关品牌字符串

尽量收口到少数几个“中心入口”里。也就是说：

**下一步要测试任意自定义品牌名时，已经不再需要全仓库盲搜盲改，而是优先改少数中心文件，再做一轮定向验证。**

不过当前还不是“单一文件驱动所有品牌字段”的最终形态，仍然存在几个独立入口需要同步维护，这会在“剩余风险”一节里说明。

---

## 2. 品牌自定义的中心入口文件

下面这些文件是**真正决定品牌值**的核心入口。后面大部分运行时/UI/脚本逻辑，都已经改为依赖这些入口，而不是直接硬编码 `RenderDoc` / `renderdoc`。

### 2.1 C++ 运行时与 UI 的中心入口

| 路径 | 当前角色 | 控制范围 | 备注 |
| --- | --- | --- | --- |
| `renderdoc/common/brand_config.h` | C++ 侧品牌默认值总入口 | 产品名、UI/CLI 名称、DLL/SHIM 名、helper 命令名、崩溃/更新/全局 hook 名、capture 扩展名/类型/magic、URL、Python 模块名、Vulkan/Android 品牌字符串等 | **最核心的品牌头文件**；改这里会影响 `renderdoc/`、`qrenderdoc/`、`renderdoccmd/` 多数运行时逻辑 |

这份头文件目前承担了“**C++ 编译期品牌配置中心**”的角色。

它的逻辑含义可以简单理解为：

- **品牌文本**：`RDOC_BRAND_PRODUCT_NAME`、`RDOC_BRAND_UI_DISPLAY_NAME`、`RDOC_BRAND_CMD_DISPLAY_NAME`
- **二进制名/查找名**：`RDOC_BRAND_CORE_DLL_NAME`、`RDOC_BRAND_UI_EXECUTABLE`、`RDOC_BRAND_CMD_EXECUTABLE`、`RDOC_BRAND_SHIM_DLL_32/64`
- **运行时辅助命令与路径**：`RDOC_BRAND_UPGRADE_COMMAND`、`RDOC_BRAND_CRASH_HANDLE_COMMAND`、`RDOC_BRAND_GLOBAL_HOOK_COMMAND`
- **capture 相关格式入口**：`RDOC_BRAND_CAPTURE_EXTENSION`、`RDOC_BRAND_CAPTURE_FILETYPE`、`RDOC_BRAND_CAPTURE_MAGIC_CHAR_*`
- **线上资源入口**：`RDOC_BRAND_WEBSITE_URL`、`RDOC_BRAND_DOCUMENTATION_URL`、`RDOC_BRAND_SOURCE_URL`、`RDOC_BRAND_ISSUES_URL`
- **平台/生态名称**：`RDOC_BRAND_VULKAN_LAYER_NAME`、`RDOC_BRAND_ANDROID_PACKAGE_BASE`、`RDOC_BRAND_PY_CORE_MODULE_NAME`

### 2.2 Windows / MSBuild 产物命名入口

| 路径 | 当前角色 | 控制范围 | 备注 |
| --- | --- | --- | --- |
| `util/Branding.props` | VS / MSBuild 品牌属性入口 | Windows 产物名、目标名、Vulkan layer 输出 JSON 替换参数、Python 模块目标名、shim 目标名等 | 被多个 `.vcxproj` 导入，是 **Windows 输出文件名** 的中心入口 |

当前导入 `util/Branding.props` 的项目包括：

- `renderdoc/renderdoc.vcxproj`
- `renderdoccmd/renderdoccmd.vcxproj`
- `renderdocshim/renderdocshim.vcxproj`
- `qrenderdoc/qrenderdoc_local.vcxproj`
- `qrenderdoc/renderdocui_stub.vcxproj`
- `qrenderdoc/Code/pyrenderdoc/pyrenderdoc_module.vcxproj`
- `qrenderdoc/Code/pyrenderdoc/qrenderdoc_module.vcxproj`
- `renderdoc/driver/vulkan/renderdoc_vulkan.vcxproj`

这份文件的逻辑含义是：

- **决定 VS 构建产物最终叫什么名字**
- **决定 Windows 下 `.dll/.exe/.pyd` 是否跟品牌同步**
- **决定 Vulkan layer JSON 生成时会替换成什么名字**

如果只改 `brand_config.h`、不改这里，运行时显示名和 Windows 实际产物名就可能出现不一致。

### 2.3 WiX 安装器品牌入口

| 路径 | 当前角色 | 控制范围 | 备注 |
| --- | --- | --- | --- |
| `util/installer/Branding.wxi` | 安装器品牌属性入口 | 安装器显示名、文件关联描述、安装目录名、APK 名、帮助链接、文档名、Capture ProgID 等 | 当前本地无 WiX 工具链，但如果将来要打安装包，这里必须同步 |

它主要解决的是：

- 安装器 UI 显示什么产品名
- 安装目录叫啥
- `.rdc` / `.cap` 文件关联文案是什么
- Start Menu / 文档快捷方式怎么显示
- Android APK 文件名如何命名

### 2.4 Python / Docs / Test 侧品牌入口

| 路径 | 当前角色 | 控制范围 | 备注 |
| --- | --- | --- | --- |
| `util/rdoc_brand.py` | Python / docs / tests / Phase 4 compat 输入入口 | Python 模块名、文档 URL、源码 URL、issue URL、analytics/bugreport URL、Android 包名、测试脚本引用，以及 `RDOC_COMPAT_*` 兼容层元数据 | **脚本层品牌总入口**，并支持用环境变量快速覆盖 |

这份文件的特点是：

- 默认值已经对齐当前仓库的品牌默认值
- 支持用环境变量覆盖，例如：
  - `RDOC_PRODUCT_NAME`
  - `RDOC_BASE_NAME`
  - `RDOC_PY_CORE_MODULE_NAME`
  - `RDOC_SOURCE_URL`
  - `RDOC_CMD_ANDROID_PACKAGE_BASE`
  - `RDOC_COMPAT_BASE_NAME`
  - `RDOC_COMPAT_APP_HEADER_NAME`
  - `RDOC_COMPAT_GETAPI_NAME`
  - `RDOC_COMPAT_REPLAY_MARKER_NAME`
- 文档脚本、测试脚本，以及 `Phase 4` 的 compat 生成脚本都尽量改成依赖这里

因此它很适合做：

- **脚本层快速试牌**
- **docs/test 不改源码先做环境覆盖验证**
- **Phase 4 兼容头 / 导出别名 / replay marker 的集中试牌**
- **未来进一步演进成更正式的 manifest/config 输入层**

### 2.5 CMake / 跨平台产物入口

| 路径 | 当前角色 | 控制范围 | 备注 |
| --- | --- | --- | --- |
| `CMakeLists.txt` | CMake 构建侧品牌入口 | CMake 输出名、Android 包名、Activity/Application Label、Vulkan 层描述、`RDOC_BASE_NAME`/`RDOC_BASE_NAME_UPPER` 注入等 | 如果要做 CMake / Android / Linux / macOS 方向的品牌切换，这里必须同步 |

当前 `CMakeLists.txt` 已经承担了：

- `RDOC_BASE_NAME`
- `RDOC_PRODUCT_NAME`
- `RDOC_CMD_NAME`
- `RDOC_CMD_DISPLAY_NAME`
- `RDOC_PY_CORE_MODULE_NAME`
- `RDOC_PY_GUI_MODULE_NAME`
- `RDOC_ANDROID_PACKAGE_BASE`
- `RDOC_VULKAN_LAYER_DESCRIPTION`

等变量的构建侧入口。

**但需要注意：这些变量当前还不是一个完全成熟的“外部可传参 branding manifest 接口”，更多还是中心化后的默认值入口。**

### 2.6 API 兼容层入口（已收口为模板 + 生成）

| 路径 | 当前角色 | 控制范围 | 备注 |
| --- | --- | --- | --- |
| `util/generate_brand_compat.py` | Phase 4 compat 生成入口 | 根据 `util/rdoc_brand.py` 中的 `RDOC_COMPAT_*` 字段生成 compat 头、compat 配置头、`rdocself.version` | **Phase 4 兼容层总开关** |
| `renderdoc/api/app/compat_app.h.in` | App API compat 模板 | 生成 `<compat>_app.h` | 当前默认生成 `ripperk_app.h` |
| `renderdoc/api/replay/compat_replay.h.in` | Replay API compat 模板 | 生成 `<compat>_replay.h` | 当前默认生成 `ripperk_replay.h` |
| `renderdoc/api/replay/compat_config.h.in` | Compat 元数据模板 | 生成 `compat_config.h`，供导出别名/replay marker/动态查找消费 | 运行时代码直接依赖它 |
| `renderdoc/rdocself.version.in` | self-capture 导出脚本模板 | 生成 `rdocself.version` 中的 compat `GetAPI` 导出 | Linux/Posix 导出链路的一部分 |
| `renderdoc/api/app/ripperk_app.h` | 当前 compat App 头生成产物 | `RIPPERK_*` 到 `RENDERDOC_*` 的别名映射 | 默认 compat 品牌仍是 `ripperK` |
| `renderdoc/api/replay/ripperk_replay.h` | 当前 compat Replay 头生成产物 | `RIPPERK_*` 到 `RENDERDOC_*` 的别名映射 | 同上 |

这一层现在的意义已经不再是“手写一个 `ripperK` 专用壳”，而是：

- 先把 compat 元数据收口到 `RDOC_COMPAT_*`
- 再用模板生成 compat 头、compat 配置头和 `rdocself` 导出脚本
- 同时保留 RenderDoc 现有 ABI / 现有导出逻辑，不一次性打断生态

因此如果你的目标是“**任意自定义品牌都能得到对应 API 头名 / 导出别名 / replay marker**”，当前已经具备这条生成链；但它**还不是自动挂进构建系统的全自动链路**，改完 `RDOC_COMPAT_*` 后需要手动执行：

- `python3 util/generate_brand_compat.py`

---

## 3. 已经收口的品牌消费点（按逻辑分组）

这一节不是“都要改”的文件，而是说明：**上面的中心入口改完后，当前哪些消费点会跟着生效。**

### 3.1 runtime lookup / helper path / stub / crash / hook

这一组已经基本从硬编码文件名，收口到 `renderdoc/common/brand_config.h`：

- `renderdoccmd/renderdoccmd_win32.cpp`
- `renderdoc/os/win32/win32_process.cpp`
- `renderdoc/os/win32/sys_win32_hooks.cpp`
- `qrenderdoc/renderdocui_stub.cpp`
- `renderdoc/core/crash_handler.h`
- `qrenderdoc/Windows/Dialogs/UpdateDialog.cpp`
- `qrenderdoc/Code/qrenderdoc.cpp`

逻辑含义：

- UI / cmd / core / shim 的运行时查找名
- `upgrade` / `crashhandle` / `globalhook` 等 helper 命令名
- 崩溃事件名、临时目录名、恢复 `.reg` 文件名
- 更新器替换时要寻找/复制的可执行文件与 DLL 名

### 3.2 Windows / UI 显示文本与静态 Qt 文本

这一组已经大面积改为依赖 `brand_config.h` 或统一的 UI 替换逻辑：

- `qrenderdoc/Code/QRDUtils.cpp`
- `qrenderdoc/Windows/MainWindow.cpp`
- `qrenderdoc/Windows/Dialogs/AboutDialog.cpp`
- `qrenderdoc/Windows/Dialogs/CaptureDialog.cpp`
- `qrenderdoc/Windows/Dialogs/CrashDialog.cpp`
- `qrenderdoc/Windows/Dialogs/ExtensionManager.cpp`
- `qrenderdoc/Windows/Dialogs/LiveCapture.cpp`
- `qrenderdoc/Windows/Dialogs/TipsDialog.cpp`
- `qrenderdoc/Windows/Dialogs/UpdateDialog.cpp`
- `qrenderdoc/Widgets/BufferFormatSpecifier.cpp`
- `qrenderdoc/Resources/qrenderdoc.rc`

其中 `qrenderdoc/Code/QRDUtils.cpp` 的 `ApplyBrandingToUIString()` 目前相当于一个**UI 文本兜底替换层**，负责把仍然存在于静态文本/HTML 里的：

- `RenderDoc`
- `QRenderDoc`
- `RenderDocCmd`
- `https://renderdoc.org`
- `https://github.com/baldurk/renderdoc`
- `baldurk@baldurk.org`

替换成当前品牌对应值。

### 3.3 Python 绑定 / Python Shell / 模块兼容名

这一组主要依赖 `brand_config.h` 与 `util/rdoc_brand.py`：

- `qrenderdoc/Code/pyrenderdoc/PythonContext.cpp`
- `qrenderdoc/Code/pyrenderdoc/interface_check.h`
- `qrenderdoc/Windows/PythonShell.cpp`
- `docs/regenerate_stubs.py`
- `docs/verify-docstrings.py`
- `util/test/run_tests.py`
- `util/test/rdtest/runner.py`
- `util/test/rdtest/util.py`
- `util/test/rdtest/remoteserver.py`

当前逻辑特点：

- 可把嵌入式 Python 初始化模块名切到自定义名
- **如果自定义名不是 `renderdoc` / `qrenderdoc`，当前仍会额外把旧模块名注册为 alias**
- 这意味着：
  - 新品牌模块名可测
  - 旧脚本通常不至于立刻全坏
  - 但这也意味着它还不是“彻底去 RenderDoc 名字暴露”的最终形态

### 3.4 capture 扩展名 / 文件类型 / magic / 打开保存过滤器

这一组已经大体依赖 `brand_config.h`：

- `renderdoc/serialise/rdcfile.cpp`
- `renderdoc/replay/capture_file.cpp`
- `renderdoc/core/core.cpp`
- `renderdoccmd/renderdoccmd.cpp`
- `qrenderdoc/Code/CaptureContext.cpp`
- `qrenderdoc/Code/ReplayManager.cpp`
- `qrenderdoc/Widgets/ReplayOptionsSelector.cpp`
- `qrenderdoc/Windows/Dialogs/CaptureDialog.cpp`
- `qrenderdoc/Windows/Dialogs/CrashDialog.cpp`
- `qrenderdoc/Windows/MainWindow.cpp`
- `renderdoc/os/win32/win32_shellext.cpp`

逻辑含义：

- capture 文件扩展名
- capture settings 扩展名
- 打开/保存对话框过滤器
- `OpenFile()` 使用的 file type
- capture 文件头 `magic`

这一层已经“可配置”，但一旦真正改值，兼容性风险仍然很高。

### 3.5 Vulkan / Android / 跨平台名称

这一组主要依赖 `brand_config.h`、`CMakeLists.txt`、`util/Branding.props`：

- `renderdoc/driver/vulkan/vk_layer.cpp`
- `renderdoc/driver/vulkan/vk_layer_android.cpp`
- `renderdoc/driver/vulkan/vk_posix.cpp`
- `renderdoc/driver/vulkan/renderdoc.json`
- `renderdoccmd/CMakeLists.txt`
- `renderdoccmd/android/AndroidManifest.xml`
- `renderdoccmd/android/Loader.java`
- `renderdoccmd/renderdoccmd_android.cpp`
- `renderdoc/android/android.cpp`
- `util/test/demos/test_common.cpp`

逻辑含义：

- Vulkan layer 名称、描述、导出函数名拼接
- Android 包名、APK 名、动态库名、intent extra key
- 测试/demo 里对 Android 包名和捕获库名的引用

### 3.6 文档 / URL / 帮助系统 / 测试脚本

这一组主要依赖 `util/rdoc_brand.py` 与 `brand_config.h`：

- `docs/conf.py`
- `docs/regenerate_stubs.py`
- `docs/verify-docstrings.py`
- `docs/getting_started/*.rst`
- `docs/how/*.rst`
- `docs/in_application_api.rst`
- `docs/introduction.rst`
- `docs/window/*.rst`
- `util/test/run_tests.py`
- `util/test/rdtest/*.py`

逻辑含义：

- 文档项目名、标题、collection name
- Python API 文档模块名
- 文档里的源码/issue/docs/analytics/bugreport 链接
- 测试脚本对模块名、Android 包名、库名的引用

---

## 4. Phase 1~8 完成情况汇总

> 说明：这里按照当前分支的**实际落地结果**总结，而不是完全照搬最初计划书的原始粒度。

| Phase | 当前状态 | 关键提交 | 完成内容摘要 |
| --- | --- | --- | --- |
| Phase 1 | 已完成 | `92cc4437c` | 建立 `renderdoc/common/brand_config.h`，把一批运行时品牌标识从散落硬编码收口到中心头文件 |
| Phase 2 | 已完成 | `3b387e142`、`a46981f66` | 升级 VS 工程到 v143；安装器身份默认值抽象，`util/installer/Branding.wxi` 成为安装器品牌中心入口 |
| Phase 3 | 已完成 | `2e11deb78` ~ `1e937e792` | 完成 runtime lookup / helper path / core dll / UI stub / global hook / crash 相关名称收口，默认查找顺序不变 |
| Phase 4 | 已完成（已收口为模板 + 生成兼容层） | `b2f22cc1f`、`50bbdd433` | 将 compat App/Replay 头、GetAPI alias、replay marker、`rdocself` 导出脚本收口到模板与生成脚本，默认 ABI / 默认行为不变 |
| Phase 5 | 已完成 | `d36eefac8` | Vulkan layer、Android 包名、Android 捕获库名、相关构建与生成逻辑收口 |
| Phase 6 | 已完成 | `b70ab2e66` | 文档构建名、安装器文案、部分路径/帮助入口抽象，形成 docs/install 方向的中心化入口 |
| Phase 7 | 已完成 | `1957701cb` | capture 扩展名 / 文件类型 / 文件头 magic / 相关 UI 过滤器与打开逻辑改为品牌配置驱动 |
| Phase 8 | 已完成 | `f197ef9ff` | 收口剩余 UI、Python、docs、tests、脚本品牌残留；新增 `util/rdoc_brand.py` 作为 Python/docs/test 层 facade |

### 4.1 各 Phase 的实际意义

#### Phase 1

核心产出：**有了品牌中心头文件**。

这一步让后续改造不再依赖“全仓库搜索替换”，而是先集中定义品牌字段，再逐步把调用方迁过去。

#### Phase 2

核心产出：**安装器身份与 Windows 构建基础入口开始独立出来**。

这一步主要是为后续“改产物名/安装器名但不影响默认行为”打基础。

#### Phase 3

核心产出：**runtime lookup / helper path / stub / crash / hook 这条高风险链路完成收口**。

这是当前你后续试品牌时最关键的一个阶段，因为：

- UI 找 cmd
- cmd 找 UI
- 进程注入找 core dll / shim
- 崩溃处理找 helper
- 更新器替换文件

这些如果还靠硬编码，改品牌时最容易直接跑不起来。

#### Phase 4

核心产出：**把 API 兼容性问题从“手写 alias”升级到“模板 + 生成 alias”**。

当前 compat 头文件、`GetAPI` alias、replay marker 与 `rdocself` 导出脚本已经可以由 `RDOC_COMPAT_*` 驱动生成，不再需要手工维护一份 `ripperK` 专用兼容壳；不过它仍然是**手动生成后再提交/构建**的链路，而不是完全自动挂进构建系统。

#### Phase 5

核心产出：**Vulkan / Android 这两条独立生态链开始可控**。

这一步避免了将来只改 Windows 名称、但 Vulkan layer / Android 侧仍然暴露 RenderDoc 的问题。

#### Phase 6

核心产出：**安装器与 docs 的外围身份开始收口**。

这一步对于“对外发布形象”非常重要，但对本地 Windows 运行时影响相对较小。

#### Phase 7

核心产出：**capture 格式相关标识已经可改，但风险仍高**。

这一阶段完成的是“让扩展名/magic 有配置入口”，不是“风险已经消失”。

#### Phase 8

核心产出：**最后一层人类可见与脚本可见品牌残留得到集中处理**。

新增 `util/rdoc_brand.py` 后，docs / tests / Python 相关逻辑终于有了统一入口，而不是到处散落旧模块名和旧 URL。

---

## 5. 剩余风险与未完全抽象之处

这部分非常关键，因为它决定了“现在能不能立刻试任意品牌”和“试牌时最容易踩哪类坑”。

### 5.1 仍然不是单一配置源

当前品牌配置至少分散在以下几个中心文件：

- `renderdoc/common/brand_config.h`
- `util/Branding.props`
- `util/installer/Branding.wxi`
- `util/rdoc_brand.py`
- `CMakeLists.txt`

其中 `Phase 4` 的 compat 链已经基本收进 `util/rdoc_brand.py -> util/generate_brand_compat.py -> *.in / 生成产物`，但 C++ 运行时主品牌仍然以 `renderdoc/common/brand_config.h` 为中心，因此**全仓库仍不是单一事实源**。

这比过去已经好很多，但**还不是“一份 manifest 驱动全仓库”的最终形态**。

风险：

- 改了 C++ 默认值，但忘了改 Windows 产物名
- 改了 Python/docs/test 入口，但忘了改运行时二进制查找名
- 改了安装器文案，但产物实际名字没同步

### 5.2 Phase 4 已基本通用化，但仍是手动生成链路

当前 Phase 4 的 compat 元数据已经收口到：

- `util/rdoc_brand.py` 中的 `RDOC_COMPAT_*`
- `util/generate_brand_compat.py`
- `renderdoc/api/app/compat_app.h.in`
- `renderdoc/api/replay/compat_replay.h.in`
- `renderdoc/api/replay/compat_config.h.in`
- `renderdoc/rdocself.version.in`

这意味着：

- 如果你的目标是“把 API 头文件名 / 导出符号 / replay marker 也切成任意品牌”，当前**已经有可用的通用化入口**
- 当前默认仍会生成 `ripperk_app.h` / `ripperk_replay.h` / `RIPPERK_GetAPI` / `ripperk__replay__marker`，只是它们不再需要手工维护

但这条链路还有一个现实风险：

- **它还没有自动挂进 VS / CMake 构建前步骤**
- 也就是说，改完 `RDOC_COMPAT_*` 后，需要先手动执行 `python3 util/generate_brand_compat.py`
- 然后再进行构建、回归，并确认生成产物已经同步刷新后再提交

### 5.3 capture 格式变更仍然是高风险区域

虽然 `Phase 7` 已经把下面这些入口抽象出来了：

- `.rdc`
- `.cap`
- `RDOC` magic
- 文件过滤器与 `OpenFile()` 的 file type

但真正改掉这些值仍会带来以下风险：

- 旧版 RenderDoc 无法直接识别新文件
- 周边脚本/工具可能仍默认使用 `.rdc`
- 如果连 magic 一起改，任何依赖旧格式特征的工具都会受影响
- 安装器文件关联、OpenWith、拖拽打开、shell 扩展都要再回归一次

### 5.4 UI 替换层目前有一部分仍是“字符串替换兜底”

`qrenderdoc/Code/QRDUtils.cpp` 中的 `ApplyBrandingToUIString()` 现在很有价值，但它本质上仍然是：

- 找到旧的 RenderDoc 文本
- 再替换成当前品牌值

风险：

- 如果未来上游改了某些 UI 静态文本写法，这个替换层可能漏掉
- 某些 HTML / 富文本 / resource 文本如果绕开这层，也可能残留旧品牌

### 5.5 Python 兼容层当前仍保留旧模块名 alias

`qrenderdoc/Code/pyrenderdoc/PythonContext.cpp` 现在的策略是：

- 新品牌模块名注册进去
- 如果新名字不是 `renderdoc` / `qrenderdoc`，就再把旧模块名也注册成 alias

优点：

- 老脚本不容易全部立刻失效

代价：

- 这还不是“完全去 RenderDoc 暴露”的终态
- 如果你要验证“生态层只接受新品牌，不再接受旧名”，还要再加一轮收紧策略

### 5.6 CMake 入口还更像“中心默认值”，不是成熟的外部覆盖接口

当前 `CMakeLists.txt` 虽然已经集中定义了：

- `RDOC_BASE_NAME`
- `RDOC_PRODUCT_NAME`
- `RDOC_CMD_NAME`
- `RDOC_PY_CORE_MODULE_NAME`
- `RDOC_ANDROID_PACKAGE_BASE`

但它们目前还不是完全规范化的“可直接通过一组外部参数无侵入切品牌”的接口。

也就是说：

- 当前更适合先手改这些中心默认值做试牌
- 如果后面你想要一条命令切任意品牌，还需要追加一轮构建系统抽象

---

## 6. 现在开始测试“任意自定义品牌”的建议执行步骤

这里给出的是**从当前仓库状态出发，最稳妥、最省返工的实际执行顺序**。

## 6.1 先决定这次试牌的边界

推荐把“自定义品牌试验”拆成两档：

### 档位 A：低风险品牌试牌（推荐先做）

只改：

- 产品显示名
- UI / cmd / core / shim / Python 模块 / 文档相关名字
- 官网 / 源码 / issue / 邮件等对外链接
- Windows 产物名

先不改：

- `.rdc` / `.cap`
- `RDOC` magic
- `RENDERDOC_*` 原生 API 符号族
- `ripperK` 兼容层的 API alias 设计

这能先验证：

- 自定义品牌是否能跑通 UI、注入、回放、脚本、文档、tests 这条主链
- 不会一下子把 capture 兼容性和 API 生态一起打断

### 档位 B：深度去 RenderDoc 特征试牌

在档位 A 稳定后，再考虑改：

- capture 扩展名 / settings 扩展名
- capture magic
- API 头文件 / 导出符号 / replay marker
- Vulkan layer / Android 包名的最终发布名
- 安装器文件关联与外部分发名

这个档位才是“最终品牌切换”的方向，但风险会显著上升。

## 6.2 推荐的实际修改顺序

如果你现在就要试一个任意品牌，推荐按下面顺序改：

### 第 1 步：先准备品牌矩阵

至少先定这几个字段：

- `ProductName`：显示产品名
- `BaseName`：基础小写名
- `BaseNameUpper`：大写 token 名
- `UIExecutable`
- `CMDExecutable`
- `CoreDllName`
- `ShimBaseName`
- `PyCoreModuleName`
- `PyGuiModuleName`
- `WebsiteUrl`
- `SourceUrl`
- `IssuesUrl`
- `SupportEmail`

建议约束：

- `BaseName` 统一使用小写 ASCII
- `BaseNameUpper` 与 `BaseName` 保持一一对应
- `UIExecutable` / `CMDExecutable` / `CoreDllName` 彼此命名风格统一

### 第 2 步：改 C++ 运行时中心入口

先改：

- `renderdoc/common/brand_config.h`

这一步是最关键的，因为它决定：

- 运行时互相找谁
- UI 显示什么
- capture 相关默认是什么
- URL 与帮助入口是什么

### 第 3 步：同步 Windows 产物命名

再改：

- `util/Branding.props`

这一步保证：

- VS 产物名字真的跟着变
- `qrenderdoc.exe` / `renderdoccmd.exe` / `renderdoc.dll` / `renderdocshim*.dll` 等输出名与运行时查找名一致

### 第 4 步：同步 Python / docs / tests 入口

再改：

- `util/rdoc_brand.py`

如果只是想快速验证 docs/test 脚本层，也可以先用环境变量覆盖这里的值。

如果这轮还要一起改 `Phase 4` 的 compat API 品牌，则在这里同时调整：

- `RDOC_COMPAT_BASE_NAME`
- `RDOC_COMPAT_BASE_NAME_UPPER`
- `RDOC_COMPAT_APP_HEADER_NAME`
- `RDOC_COMPAT_REPLAY_HEADER_NAME`
- `RDOC_COMPAT_GETAPI_NAME`
- `RDOC_COMPAT_REPLAY_MARKER_NAME`

改完后先执行：

```bash
python3 util/generate_brand_compat.py
```

再进入后续构建与回归。

### 第 5 步：按需同步安装器与 CMake

如果本轮还要覆盖安装器或跨平台构建，再改：

- `util/installer/Branding.wxi`
- `CMakeLists.txt`

如果这次仅做 Windows 本地试牌：

- `Branding.wxi` 可以先不作为阻塞项
- `CMakeLists.txt` 可以先不作为阻塞项

### 第 6 步：决定是否保留旧兼容入口

当前默认建议：

- 保留旧 Python 模块名 alias
- 保留现有 RenderDoc API / ripperK alias 并存策略
- 保留 `.rdc` / `.cap` / `RDOC` magic

这样最利于先完成第一轮试牌与运行验证。

---

## 7. 你接下来执行自定义品牌时的最小验证集

下面这套验证是基于你当前本地环境（Windows + MSBuild 可用）整理的，优先顺着你已经熟悉的节奏来。

### 7.1 构建验证

#### 必做

- `Development|x64` 构建
- `Release|x64` 构建

命令可继续沿用你当前已经验证过的方式：

```bat
MSBuild.exe renderdoc.sln /m /nologo /verbosity:minimal /p:Configuration=Development /p:Platform=x64
MSBuild.exe renderdoc.sln /m /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=x64
```

#### 构建后立刻检查的产物

重点看输出目录下这些文件名是否和你的品牌矩阵一致：

- core DLL
- UI EXE
- CMD EXE
- UI stub EXE
- shim32 / shim64 DLL
- `.json` Vulkan layer 文件（如果这轮覆盖到）
- Python `.pyd` 模块（如果改了模块目标名）
- compat 头 / compat 配置是否已刷新（如果这轮改了 `RDOC_COMPAT_*`）

如果本轮触及 `Phase 4` compat 名称，额外检查：

- `renderdoc/api/app/<compat>_app.h`
- `renderdoc/api/replay/<compat>_replay.h`
- `renderdoc/api/replay/compat_config.h`
- `renderdoc/rdocself.version`

确认它们已经由 `python3 util/generate_brand_compat.py` 同步刷新。

### 7.2 运行时验证

#### 必做

1. 启动 UI，确认主窗口标题、About、Tips、Update 相关文本没有明显旧品牌残留。
2. 打开一个已有 capture，确认可正常加载。
3. 执行一次你已经熟悉的 `Launch + Inject` 64 位目标流程。
4. 确认能正常关闭并重新打开 UI，不出现 helper / stub / crash 路径找不到的问题。
5. 在 Python shell 中验证模块导入：
   - 新模块名是否可导入
   - 旧 `renderdoc` / `qrenderdoc` alias 是否仍可导入（如果你本轮保留兼容）

#### 推荐补充

- 执行 CLI 帮助：`<你的新 cmd>.exe --help`
- 执行版本查看：`<你的新 cmd>.exe version`
- 验证一次更新器相关 UI 能正常拉起（不要求真更新）

### 7.3 如果这轮改了 capture 扩展名 / magic，还要额外做的验证

只有在你决定动 `Phase 7` 的真实值时，才需要增加下面这些检查：

1. 新建 capture，确认保存时扩展名正确。
2. 从 UI 打开新扩展名 capture，过滤器正确。
3. `renderdoccmd` 打开/转换 capture 正常。
4. 拖拽打开 / shell 打开（如果本地有相关集成）正常。
5. 旧 `.rdc` 是否仍需要兼容读取；如果需要，就要明确做一次兼容验证。

### 7.4 如果这轮改了 Vulkan / Android 名称，还要额外做的验证

1. 检查输出的 layer JSON 名、layer 名、描述是否同步。
2. 检查 Android 包名、APK 名、intent extra key 是否一致。
3. 做一轮最小 Android 连接 / 包识别验证（如果这次范围包含 Android）。

---

## 8. 我对“下一步自定义品牌试验”的具体建议

如果你现在的目标是：

> 先验证“这个 fork 是否已经具备切到任意品牌的基础能力”

我建议下一轮**不要一步到位把所有协议层和格式层都改掉**，而是：

### 推荐的第一轮试牌范围

- 改 `renderdoc/common/brand_config.h`
- 改 `util/Branding.props`
- 改 `util/rdoc_brand.py`
- 暂时不改 `RDOC_BRAND_CAPTURE_EXTENSION` / `RDOC_BRAND_CAPTURE_MAGIC_CHAR_*`
- 暂时不去动 `RIPPERK_*` 兼容层，把它视为现阶段额外兼容桥

这样能最快回答两个关键问题：

1. **品牌抽象是否已经足够支撑 UI / runtime / helper / python / docs / tests 的联动切换？**
2. **现在还剩下哪些是“通用化不够”的点，而不是“搜索替换漏改”的点？**

### 如果这轮通过，下一轮再做什么

如果第一轮任意品牌试牌通过，下一轮最值得做的是：

1. 把 `Phase 4` 从 `ripperK` 专用兼容层，进一步抽象成更通用的品牌/API alias 生成方案。
2. 把 `CMakeLists.txt` / `Branding.props` / `Branding.wxi` / `brand_config.h` / `rdoc_brand.py` 再往“单一 manifest 驱动”收一步。
3. 再决定是否真的要切 `.rdc` / `RDOC` magic 这些高风险格式标识。

---

## 9. 一句话结论

**到 `f197ef9ff` 为止，Phase 1~8 已经把“品牌散落硬编码”大体收口成了“少数中心入口 + 大量消费点跟随”的结构。**

这意味着你现在已经可以开始做“任意自定义品牌”的**第一轮低风险试牌**，但要注意：

- 当前还不是单一配置源
- Phase 4 仍然带有 `ripperK` 专用兼容层色彩
- capture 格式/API/协议层的彻底换名仍然是高风险区

如果你的目标是**先验证品牌切换基础能力**，现在已经具备可执行条件；
如果你的目标是**一步做到完全去 RenderDoc 特征**，那还需要在 API 通用化和单一配置源这两件事上继续收口。
