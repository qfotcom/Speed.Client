# SpeedClient

Speed.Server 示例客户端（**REST** + **Legacy 订阅推送**），UI 使用与 [SenseWorkflow](../SenseWorkflow) 相同的 **SenseDesign 2.x（shadcn QML）** 子模块。

## 结构

| 路径 | 说明 |
|------|------|
| `src/sdk/` | `speed_client_sdk`：Legacy 帧协议、REST HTTP、QSettings 配置 |
| `src/app/` | Qt Quick 应用 `SpeedClient`（`ShadcnAppWindow` + 三页测试） |
| `third_party/SenseDesign` | **Git 子模块**（SenseDesign 2.x / shadcn QML），与 SenseWorkflow 相同约定 |
| `android/` | Android 包清单（明文 HTTP、INTERNET） |

旧版 Tabler `SenseAppWindow` / `Sense*` 组件已移除；外壳为 **`ShadcnAppWindow`**（`SenseAppShell`），页面为 **连接 / REST / 推送** 三页结构不变。

## 初始化 SenseDesign 子模块

克隆本仓库后 **必须** 拉取子模块（CMake 只认 `third_party/SenseDesign`）：

```powershell
.\tools\setup_submodules.ps1
# 需要跟踪 shadcn 分支最新提交时：
.\tools\setup_submodules.ps1 -FetchSenseDesignBranch
```

升级 UI 库：在 `third_party/SenseDesign` 内 pull 或 bump 指针后 `git add third_party/SenseDesign` 并提交。

## 构建

### 桌面（Windows）

在 Qt Creator 中打开本目录 `CMakeLists.txt`，运行目标选 **SpeedClient**。

```powershell
cmake -B build -DCMAKE_PREFIX_PATH="<Qt6 安装路径>"
cmake --build build --config Release
```

### Android

选择 **Qt 6.x for Android arm64-v8a** Kit，构建 **SpeedClient**（Debug 便于签名安装）。

- 包名：`com.speed.client`
- 模拟器访问本机：`host = 10.0.2.2`
- 真机：PC 局域网 IP

**服务端**：Legacy/REST 需监听 `0.0.0.0`。

#### 安装失败 `INSTALL_PARSE_FAILED_NO_CERTIFICATES`

Release 为未签名 APK 时不能安装；联调请用 **Debug** 或为 Release 配置 Keystore（与 SenseDesign / SenseWorkflow 相同流程）。

#### 闪退 / logcat

启动日志：`files/speed_startup.log`；logcat 标签 **`SpeedClient`**。

```powershell
powershell -File tools/android_debug_debug.ps1 -Serial <device>
```

## 联调 Speed.Server

见仓库内 `configs/` 与 [Speed.Server](../Speed.Server) README。默认 Legacy **9001**、REST **8080**。

Workflow 模式下 **PUSH 随下一次响应帧下发**；在「推送」页开启 **轮询 PING** 可及时收到事件。
