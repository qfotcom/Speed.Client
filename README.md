# SpeedClient

Speed.Server 示例客户端（第一期：**REST** + **Legacy 订阅推送**），UI 基于 [SenseDesign](../SenseDesign)。

## 结构

| 路径 | 说明 |
|------|------|
| `src/sdk/` | `speed_client_sdk`：Legacy 帧协议、REST HTTP、QSettings 配置 |
| `src/app/` | Qt Quick 应用 `SpeedClient` |
| `src/app/`（`Main.qml`、`pages/`、`components/`） | 界面（连接 / REST / 推送） |
| `android/` | Android 包清单（明文 HTTP、INTERNET） |
| `configs/` | 示例配置说明 |

SenseDesign 通过 CMake `add_subdirectory(../SenseDesign)` 引入，不复制 UI 库源码。应用壳使用 **`SenseAppShell`** 模块（`SenseAppWindow` + `SenseAppPageLayout`），与 SenseDesign Demo 共用 Tabler 布局。

## 构建

### 桌面（Windows）

在 Qt Creator 中打开本目录 `CMakeLists.txt`，或：

```powershell
cmake -B build -DCMAKE_PREFIX_PATH="<Qt6 安装路径>" -DSENSEDESIGN_ROOT="../SenseDesign"
cmake --build build --config Release
```

### Android

与 SenseDesign 相同：选择 **Qt 6.x for Android arm64-v8a** Kit，配置 SDK/NDK/JDK 后直接构建 `SpeedClient`。

- 包名：`com.speed.client`
- 模拟器访问本机服务：`host = 10.0.2.2`（应用默认）
- 真机：填写 PC 局域网 IP

**服务端**：`configs/server.json` 中 Legacy/REST 建议监听 `0.0.0.0`，仅 `127.0.0.1` 时手机无法连入。

#### 安装失败 `INSTALL_PARSE_FAILED_NO_CERTIFICATES`

Release 构建产物为 `*-release-unsigned.apk`，**未签名不能安装**。与 [SenseDesign Android 说明](../SenseDesign/README.md) 相同：

1. **日常联调（推荐）**：在 Qt Creator 左下角把构建配置改成 **Debug**（Android Debug Kit），再点运行。会用默认 debug 密钥签名并安装。
2. **坚持用 Release 装真机**：**编辑 → Preferences → 设备 → Android**（或 **Projects → 你的 Android Kit → Android Settings**）配置 **Keystore**（路径、别名、store/key 密码）；或在 **Projects → 构建 → Build Android APK** 勾选 **Sign package**。可与 SenseDesign 共用同一套 `*.keystore`。
3. **尚无密钥库**（密码自行替换）：
   ```text
   keytool -genkey -v -keystore speedclient-debug.keystore -alias speedclient -keyalg RSA -keysize 2048 -validity 10000
   ```

当前报错路径里的 `android-build-SpeedClient-release-unsigned.apk` 即未签名 Release；改用 Debug 运行，或为 Release 配置签名后重新构建再部署。

#### 闪退 / logcat 为空

启动日志：`files/speed_startup.log`，并镜像到  
`Android/data/com.speed.client/files/Download/SpeedClient/speed_startup.log`。  
logcat 标签 **`SpeedClient`**。

在仓库根目录执行（`-Serial` 改成你的 `adb devices` 地址）：

```powershell
powershell -File tools/android_debug_debug.ps1 -Serial 192.168.1.3:5555
powershell -File tools/android_debug_release.ps1 -Serial 192.168.1.3:5555
powershell -File tools/android_debug.ps1
```

脚本会：清 logcat → 启动应用 → 拉 startup 日志 → 输出 **崩溃相关 logcat**（FATAL / SIGSEGV / Qt / QML 等）。

## 联调 Speed.Server

```powershell
cd ..\Speed.Server
cmake --preset windows-vs2019-x64-grpc
cmake --build build/vs2019-x64 --config Release
.\build\vs2019-x64\Release\speed_server.exe configs\server.json
```

默认端口（以你的 `server.json` 为准）：Legacy **9001**，REST **8080**。

### REST

- `GET /health`
- `GET /api/v1/echo?text=world`

### Legacy

- `PING` → `PONG`
- `REQ echo echo world` → `OK hello: world`
- `SUB quote.test` → `OK`
- 服务端 `PUB quote.test data` 或业务推送 → 客户端日志 `[PUSH]`

Workflow 模式下 **PUSH 随下一次响应帧下发**；在「推送」页开启 **轮询 PING** 可及时收到事件。

## 后续

gRPC（`Echo` / `Gateway.Invoke`）计划在第二期接入（Qt Grpc 或 Android arm64 预编译 gRPC）。
