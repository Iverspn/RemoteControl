<<<<<<< HEAD
# RemoteControl - 远程桌面控制工具

基于 **Qt 6 + C++17** 开发的 Windows 远程桌面控制软件，通过 TCP 协议传输 JPEG 压缩画面，支持鼠标、键盘远程操控和文件传输。

---

## 功能特性

### 主机模式（被控端）
- 监听指定端口，等待客户端连接
- 自动生成 **6 位设备 ID** 和 **6 位随机密码**，支持重新生成密码
- 屏幕实时捕获，JPEG 压缩后传输
- **自适应缩放**：高帧率时自动降低分辨率以保持流畅
- 异步 JPEG 编码，不阻塞 UI 线程
- 单客户端连接，拒绝重复连接
- 接收并注入远端鼠标键盘事件（Win32 `SendInput` API）
- 心跳保活：30 秒 ping / 120 秒超时断开

### 客户端模式（控制端）
- 通过 IP、端口、设备 ID、密码连接主机
- 实时显示远端桌面画面（异步解码）
- 鼠标点击、移动、滚轮远程操控
- 键盘输入转发（Win32 虚拟键码）
- 画质（10-100）和帧率（1-30 FPS）实时调节
- 文件传输（发送 / 接收）
- 断线自动重连（3 秒间隔）
- 连接超时提示（5 秒）

### 传输优化
- 鼠标移动事件合并（2ms 批次发送），避免高频小包
- 帧传输背压保护（2MB 缓冲区阈值），防止网络拥塞
- TCP 低延迟选项 + 256KB 收发缓冲区
- 缓冲区上限保护（128MB），防止内存溢出

---

## 系统要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10 / 11（仅支持 Windows） |
| 编译器 | MSVC 2019+ 或 MinGW-w64（支持 C++17） |
| Qt 版本 | Qt 6.x（core, gui, widgets, network, concurrent） |
| 依赖 | Win32 SDK (`user32.lib`) |

---

## 编译与运行

### 1. 安装 Qt 6

从 [Qt 官网](https://www.qt.io/download) 下载 Qt 6.x，安装时勾选：
- MSVC 2019/2022 64-bit 或 MinGW 64-bit
- Qt Network Authorization（可选）

### 2. 编译项目

```bash
# 进入项目目录
cd RemoteControl_JPEG_UP/RemoteControl

# 使用 qmake 生成 Makefile
qmake RemoteControl.pro

# 编译（MSVC）
nmake

# 或编译（MinGW）
mingw32-make
```

也可以在 **Qt Creator** 中直接打开 `RemoteControl.pro` 文件，点击构建。

### 3. 运行

编译完成后，在 `release/` 或 `debug/` 目录下找到 `RemoteControl.exe`，双击运行。

解压缩包，也可以直接在 `release/`目录下找到 `RemoteControl.exe`，双击运行。

---

## 使用指南

### 主界面

启动后进入主界面，显示当前设备的 **6 位设备 ID**，可选择：

- **创建主机** — 进入主机模式，作为被控端
- **加入主机** — 进入客户端模式，作为控制端

### 作为被控端（Host）

1. 在主界面点击 **「创建主机」**
2. 确认并记录以下信息（需告知控制端）：
   - **设备 ID**：6 位设备标识
   - **端口**：默认 `19527`
   - **密码**：6 位随机密码，可点击刷新重新生成
3. 调整参数：
   - **画质**：10（低画质低带宽）～ 100（高画质），默认 35
   - **帧率**：1～30 FPS，默认 10
4. 点击 **「开始监听」**
5. 等待客户端连接，状态栏显示客户端 IP
6. 点击 **「停止监听」** 结束会话

> **注意**：请确保控制端能访问被控端的 IP 地址，防火墙已放行对应端口。

### 作为控制端（Client）

1. 在主界面点击 **「加入主机」**
2. 输入被控端信息：
   - **主机地址**：被控端 IP 地址
   - **端口**：被控端监听端口（默认 `19527`）
   - **设备 ID**：被控端显示的设备 ID
   - **密码**：被控端提供的密码
3. 点击 **「连接」**
4. 连接成功后，远端桌面画面显示在界面中
5. 在画面上操作鼠标和键盘，实时控制远端电脑
6. 可随时调节画质和帧率
7. 点击 **「断开连接」** 结束远程会话

### 文件传输

- **发送文件**：在客户端页面点击「发送文件」，选择本地文件发送到被控端
- **接收文件**：在客户端页面点击「接收文件」，从被控端下载文件

---

## 项目结构

```
RemoteControl_JPEG_UP/
├── README.md
└── RemoteControl/
    ├── RemoteControl.pro          # Qt 项目文件
    ├── main.cpp                   # 应用入口，全局样式
    ├── resources.qrc              # Qt 资源文件
    ├── picture/
    │   ├── icon.png               # 应用图标
    │   └── background.jpg         # 背景图片
    └── src/
        ├── model/
        │   └── Protocol.h         # 协议定义（Magic Number、默认参数）
        ├── core/
        │   ├── RemoteSession.h    # 核心会话管理（主机/客户端逻辑）
        │   ├── RemoteSession.cpp
        │   ├── ScreenCapture.h    # 屏幕捕获 + JPEG 编码
        │   ├── ScreenCapture.cpp
        │   ├── InputInjector.h    # Win32 输入注入
        │   └── InputInjector.cpp
        └── ui/
            ├── MainWindow.h       # 主窗口（无边框 + 拖拽）
            ├── MainWindow.cpp
            ├── HomePage.h         # 首页（模式选择）
            ├── HomePage.cpp
            ├── HostPage.h         # 主机页面（监听设置）
            ├── HostPage.cpp
            ├── ClientPage.h       # 客户端页面（连接设置）
            ├── ClientPage.cpp
            ├── RemoteView.h       # 远程画面渲染 + 事件捕获
            └── RemoteView.cpp
```

### 核心模块说明

| 模块 | 职责 |
|------|------|
| `Protocol.h` | TCP 协议帧格式定义（JSON/FRME/FILE Magic Number） |
| `RemoteSession` | 核心控制器：管理主机/客户端状态机、认证、心跳、输入转发、文件传输 |
| `ScreenCapture` | 屏幕定时抓取 + JPEG 异步编码 + 自适应缩放 |
| `InputInjector` | Windows `SendInput` API 封装，注入鼠标键盘事件 |
| `RemoteView` | 远端画面渲染（异步 JPEG 解码）+ 本地输入事件捕获 + 坐标映射 |

---

## 通信协议

采用 **大端序（Big-Endian）二进制帧格式**：

### JSON 控制消息
```
| Magic (4B) | Length (4B) | JSON Payload |
  0x4A534F4E    payload_len    UTF-8 JSON
```

### JPEG 帧数据
```
| Magic (4B) | Length (4B) | Width (4B) | Height (4B) | JPEG Data |
  0x46524D45    payload_len    frame_w       frame_h
```

### 文件传输
```
| Magic (4B) | Length (4B) | NameLen (4B) | FileSize (8B) | FileName | FileData |
  0x46494C45    payload_len    name_len       total_size      UTF-8      binary
```

### 主要 JSON 消息

| 动作 | 方向 | 说明 |
|------|------|------|
| `auth` | Client → Host | 认证请求（设备ID + 密码） |
| `auth_success` | Host → Client | 认证通过 |
| `auth_failed` | Host → Client | 认证失败 |
| `ping` / `pong` | 双向 | 心跳保活 |
| `input_mouse_move` | Client → Host | 鼠标移动 |
| `input_mouse_event` | Client → Host | 鼠标点击+位置（合并消息） |
| `input_key_press` / `input_key_release` | Client → Host | 键盘按下/释放 |
| `input_mouse_wheel` | Client → Host | 滚轮事件 |
| `set_quality` / `set_fps` | Client → Host | 动态调整画质/帧率 |

---

## 配置参数

| 参数 | 默认值 | 范围 | 说明 |
|------|--------|------|------|
| `DEFAULT_HOST_PORT` | `19527` | 1-65535 | 主机监听端口 |
| `DEFAULT_QUALITY` | `35` | 10-100 | JPEG 压缩质量（35 为画质/速度平衡点） |
| `DEFAULT_FPS` | `10` | 1-30 | 屏幕捕获帧率 |
| 心跳间隔 | `30s` | — | Ping 发送间隔 |
| 心跳超时 | `120s` | — | 无响应断开时间 |
| 连接超时 | `5s` | — | TCP 连接超时 |
| 重连间隔 | `3s` | — | 断线自动重连间隔 |
| 背压阈值 | `2MB` | — | 发送缓冲区上限 |
| 输入合并 | `2ms` | — | 鼠标移动批次发送间隔 |

---

## 常见问题

### Q: 客户端提示"连接超时"或"连接被拒绝"？
1. 确认被控端已点击「开始监听」
2. 确认输入的 IP 地址和端口正确
3. 检查被控端防火墙是否放行了对应端口
4. 确认两台电脑在同一局域网内（或已配置端口转发）

### Q: 远程画面卡顿？
- 降低帧率（推荐 5-10 FPS）
- 降低画质（推荐 25-50）
- 检查网络带宽是否充足

### Q: 键盘输入无效？
- 确保远程画面窗口已获得焦点（点击画面区域）
- 部分系统组合键（如 Alt+Tab）可能被本地系统拦截
- 某些安全软件可能阻止 `SendInput` API

### Q: 能否跨平台使用？
- 当前版本仅支持 Windows
- **控制端**理论上可移植到其他平台（需替换 `InputInjector`）
- **被控端**依赖 Win32 API，需要较大改造

---

## 技术亮点

- **异步 JPEG 编解码**：使用 `QtConcurrent::run` 异步编码，`QFutureWatcher` 异步解码，不阻塞主线程
- **自适应缩放策略**：低 FPS 全分辨率，高 FPS 自动降分辨率，保持编码延迟可控
- **输入批处理**：高频鼠标移动事件合并为 2ms 批次，减少网络小包
- **无边框窗口**：自定义标题栏 + 拖拽移动 + 暗色主题全局样式
- **TCP 粘包处理**：完整的状态机解析器，支持 Magic Number 搜索恢复同步

---

## 开发计划

- [ ] 多显示器支持
- [ ] 剪贴板同步
- [ ] SSL/TLS 加密传输
- [ ] H.264 硬件编码（更高效的视频传输）
- [ ] 音频传输
- [ ] 多客户端同时连接
- [ ] macOS / Linux 被控端支持

---

## License

本项目仅供学习和研究使用。
=======
# RemoteControl
A simple remote control tool that can control another device on the local network
>>>>>>> ba000dcb61218c551d48b73059c64ccf5b04ab22
