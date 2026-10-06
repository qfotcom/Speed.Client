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

`Main.qml` 已开启 **`keepPagesAlive: true`**：三页切换时各自滚动与编辑状态保留；Legacy 连接状态等由 **`ClientBackend`** 全局共享，各页徽章一致。

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
- 局域网：Legacy / REST 主机均填 PC IP（模拟器常用 `10.0.2.2`），端口 **9001 / 8080**
- 跨网：使用默认 cpolar 双主机（见上表），无需改端口

**服务端**：Legacy/REST 需监听 `0.0.0.0`。

#### `Activity … QtActivity does not exist`（Activity Manager error type 3）

表示设备上 **没有安装** `com.speed.client`，但 Run 仍尝试 `am start`。常见原因：

1. **部署步骤未启用**：Qt Creator → **Projects** → Android Kit → **Release** → **Deploy** 必须选带 **Deploy to Android device**（`androiddeployqt`）的配置，不能是空的「默认部署」。
2. **只构建未安装**：构建日志里有 APK，但 Deploy 面板没有 `adb install` 成功行。
3. **安装被拒**：见下条证书错误；先卸载旧包再 Run。

联调建议左下角选 **Debug** + 上述 Android 部署配置，然后 **Build → Run**。

#### 安装失败 `INSTALL_PARSE_FAILED_NO_CERTIFICATES`

Release 为未签名 APK 时不能安装；联调请用 **Debug** 或为 Release 配置 Keystore（与 SenseDesign / SenseWorkflow 相同流程）。

#### 闪退 / logcat

启动日志：`files/speed_startup.log`；logcat 标签 **`SpeedClient`**。

```powershell
powershell -File tools/android_debug_debug.ps1 -Serial <device>
```

## 联调 Speed.Server

见 [Speed.Server/docs/speed-client-connectivity.md](../Speed.Server/docs/speed-client-connectivity.md)。

**默认（cpolar 预留 TCP，跨网）：**

| 通道 | 主机 | 端口 |
|------|------|------|
| Legacy | `1.tcp.cpolar.cn` | 20771 |
| REST | `6.tcp.cpolar.cn` | 10513 |

默认 **启动自动连接**（Legacy TCP + REST `/health`）。Legacy **断线约 250ms 内重连**；若连不上则指数退避至最多 8s。曾在推送页 **SUB** 过的 Topic 会在重连成功后自动再 SUB（及轮询 PING）；可在「连接」页关闭自动连接。局域网联调时将两路主机改为同一 PC IP，端口 **9001 / 8080** 后保存。

若界面仍显示旧版「单个主机」表单，请 **完整重新编译并安装**（Android 建议先卸载旧 APK 或 bump 安装），确保 `ClientBackend.legacyHost` / `restHost` 与新 QML 一并打进包内。

Workflow 模式下 **PUSH 随下一次响应帧下发**；在「推送」页开启 **轮询 PING** 可及时收到事件。
