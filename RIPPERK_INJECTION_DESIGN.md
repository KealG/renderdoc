# ripperK 注入链与扩展加载设计

## 1. 目标

本设计面向当前 `RenderDoc -> ripperK` 定制分支，目标是在不破坏既有捕获、重放、崩溃处理与 UI 流程的前提下，完成 Windows 注入链的统一命名、稳定化和可扩展化。当前优先级如下：

- 以 `ripperk.dll` 作为唯一核心 hook 模块，避免再保留旧 `renderdoc.dll` 回退路径。
- 保持 `qripperk.exe`、`ripperkui.exe`、`ripperkcmd.exe`、`ripperkshim32/64.dll` 的链路一致。
- 保留现有远程线程注入路径，同时新增显式可选的“代理加载模式”作为补充。
- 内核层内容仅作为研究边界和诊断扩展，不作为提权、绕过 UAC、PPL、反作弊或隐蔽注入方案。

## 2. 当前已完成基础

当前仓库已完成一轮关键重命名，统一入口位于 `renderdoc/api/replay/renderdoc_names.h`。现有核心名称如下：

- 产品名：`ripperK`
- UI：`qripperK`
- 核心 DLL：`ripperk.dll`
- UI Stub：`ripperkui.exe`
- CLI：`ripperkcmd.exe`
- Shim：`ripperkshim32.dll` / `ripperkshim64.dll`
- Replay marker：`ripperk__replay__marker`

同时，以下部分已经与新名称链路对齐：

- 根构建系统与主要 VS 工程输出名。
- `win32_process.cpp` 的注入与错误诊断。
- `renderdoccmd_win32.cpp`、`renderdocshim.h`、`sys_win32_hooks.cpp` 的核心引用。
- `qrenderdoc.cpp`、`MainWindow.cpp`、`renderdocui_stub.cpp` 的 UI 启动与提权重试逻辑。
- `crash_handler.h`、资源文件、崩溃管道名、全局 hook 数据名等。

## 3. 现有用户态注入链

当前推荐继续保留的主路径如下：

1. `qripperk.exe` 或 `ripperkcmd.exe` 选择目标进程。
2. `Process::InjectIntoProcess()` 调用 `InjectDLL()`，将 `ripperk.dll` 注入目标。
3. 目标进程完成 `LoadLibraryW` 后，沿用现有 hook、capture、target control 流程。
4. `ripperkshim32/64.dll` 继续处理子进程/全局 hook 相关场景。
5. UI 端根据 PID、错误码与远程线程退出码给出诊断信息；权限不足时允许以管理员身份重启 UI 重试。

这条链路的硬约束：

- 所有可执行名、DLL 名、pipe/event/global hook 名必须只从命名头统一派生。
- 不再保留对旧 `renderdoc*.exe/.dll` 的兼容分支。
- 注入时必须使用绝对路径，禁止依赖工作目录碰运气。
- 失败诊断至少覆盖：`OpenProcess`、`VirtualAllocEx`、`WriteProcessMemory`、`CreateRemoteThread`、等待结果、远程 `LoadLibraryW` 返回值。

## 4. 推荐扩展：Proxy Loader Mode

对于远程线程注入不稳定、目标过早退出、工作目录复杂或图形运行时天然先加载的场景，建议新增“代理加载模式”，但仅作为显式可选模式，不替代现有主路径。

### 4.1 总体思路

- 第一阶段仅支持 `vulkan-1.dll` 代理，第二阶段再评估 `dxgi.dll`。
- 代理 DLL 只负责两件事：转发到真实系统 DLL；在安全时机触发一次轻量 bootstrap。
- bootstrap 不在 `DllMain` 做重活，应在首个稳定导出调用点使用 `InitOnceExecuteOnce`。
- bootstrap 读取由 `qripperk`/`ripperkcmd` 生成的 session manifest，拿到 `ripperk.dll` 绝对路径和本次启动参数。
- bootstrap 加载 `ripperk.dll` 后，调用一个最小导出入口，让后续逻辑回到现有 RenderDoc/ripperK hook 流程。

### 4.2 建议模块划分

- `proxy/common/`
  - manifest 读取
  - 日志与诊断
  - 真实系统 DLL 解析
- `proxy/vulkan-1/`
  - `vulkan-1.dll` 导出转发
  - 首次安全调用点 bootstrap
- `renderdoc/`
  - 新增最小 bootstrap 导出，例如 `ripperK_BootstrapFromProxy(...)`
- `qrenderdoc/` 与 `renderdoccmd/`
  - 生成 manifest
  - 提供“Proxy Loader Mode” 启动选项

### 4.3 manifest 建议字段

- `version`
- `target_api`：`vulkan` / `dxgi`
- `core_dll_path`
- `working_directory`
- `capture_file_template`
- `inject_flags`
- `created_by_pid`
- `session_nonce`

manifest 缺失、字段不合法、路径不匹配时，代理 DLL 应降级为“只转发，不注入”。

## 5. 对外部案例的合理借鉴

基于已掌握信息，可以借鉴的是“自然加载点 + 二段 bootstrap”思路，而不是照搬任何面向特定游戏的项目结构。对本仓库更合理的落点是：

- 不引入面向单一游戏的硬编码目标名。
- 不把代理 DLL 做成隐蔽劫持工具，而是只用于用户显式选择的启动模式。
- 不复制第二套 hook 引擎，而是始终回到 `ripperk.dll` 这一条主链。

## 6. 内核层研究边界

如果后续确实要研究内核层，仅建议把它作为“诊断与协调扩展”，而不是直接承担注入职责。可研究但应保持高层边界的内容：

- 进程创建、镜像加载、句柄权限、完整性级别的观察模型。
- 用户态注入失败时，如何建立更强的诊断链路。
- 通过签名驱动或受控服务为用户态模块提供状态查询与日志通道。

明确不纳入本设计的内容：

- 内核 APC、手工映射、令牌窃取、UAC 绕过。
- PPL 破坏、反作弊规避、PatchGuard 对抗。
- 任何以隐蔽、持久化、绕过安全边界为目标的实现。

## 7. 稳定性验证

建议每次阶段性修改后固定执行以下检查：

- 名称扫描：`rg -n "renderdoccmd\\.exe|renderdocshim(32|64)\\.dll|renderdoc\\.dll"`
- 构建验证：x64 `Development` 与 `Release` 全量编译。
- 注入冒烟：现有进程注入、启动新进程捕获、管理员重试三条路径各跑一遍。
- 代理冒烟：manifest 存在/缺失/路径错误三类场景都验证。
- 回归点：崩溃处理、replay marker、shim 装载、UI attach、capture 保存路径。

## 8. 分阶段落地建议

- Phase 1：继续清扫残留旧标识，确保主链只认 `ripperK`。
- Phase 2：实现 `vulkan-1.dll` 代理加载模式与 manifest。
- Phase 3：把代理模式接入 `qripperk` 与 `ripperkcmd` 的 UI/CLI。
- Phase 4：补齐自动化验证与失败诊断。
- Phase 5：若仍需研究内核层，仅做签名驱动 + 诊断通道的独立实验，不进入默认产品链路。
