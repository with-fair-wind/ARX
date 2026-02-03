# MFC + ARX 二次开发项目

这是一个基于 MFC（Microsoft Foundation Classes）和 ARX（AutoCAD Runtime Extension）的 ZWCAD 二次开发项目。

## 项目结构

```
mfc/
├── CMakeLists.txt      # CMake 构建配置文件
├── inc/                # 头文件目录
│   ├── stdafx.h        # 预编译头文件
│   ├── targetver.h     # Windows 版本定义
│   └── command.h       # 命令声明
├── src/                # 源文件目录
│   ├── dllmain.cpp     # DLL 主入口和 MFC 应用程序类
│   └── command.cpp     # 命令实现
└── README.md           # 本文件
```

## 功能说明

### 已实现的命令

1. **MFCTEST** - 显示一个简单的 MFC 对话框示例
   - 演示如何创建和显示 MFC 对话框
   - 展示 MFC 控件的动态创建

2. **MFCENT** - 创建图形实体示例
   - 通过交互方式选择两个点
   - 创建一条直线实体
   - 使用 MFC 消息框进行确认

## 技术特点

- **MFC 支持**：使用共享 MFC DLL (_AFXDLL)
- **ARX 集成**：完整的 ARX API 支持
- **工具库**：依赖 arxApi 提供的工具函数
- **Unicode 支持**：全 Unicode 编码

## 构建说明

项目使用 CMake 构建系统，参考 arxApi 和 main 项目的配置方式：

1. 自动检测平台架构（x86/x64）
2. 链接 ZWCAD ZRX SDK 库
3. 依赖 arxApi 静态库
4. 输出 .zrx 格式的插件文件
5. 配置 Visual Studio 调试器指向 ZWCAD.exe

## 使用方法

1. 编译生成 mfc.zrx 文件
2. 在 ZWCAD 中使用 `ZRXLOAD` 命令加载插件
3. 执行 `MFCTEST` 或 `MFCENT` 命令测试功能

## 开发指南

### 添加新命令

1. 在 `inc/command.h` 中声明命令函数
2. 在 `src/command.cpp` 中实现命令函数
3. 在 `src/dllmain.cpp` 的 `zcrxEntryPoint` 函数中注册命令：

```cpp
acedRegCmds->addCommand(_T("MFCGROUP"), 
                        _T("MYCMD"), 
                        _T("MYCMD"), 
                        ACRX_CMD_MODAL, 
                        MyCommandFunction);
```

### 创建 MFC 对话框

参考 `command.cpp` 中的 `CTestDialog` 类，可以：
- 继承 `CDialog` 或 `CDialogEx`
- 动态创建控件（无需资源文件）
- 或使用资源编辑器创建对话框资源

### 访问 CAD 数据库

项目已包含完整的 ARX API 和 arxApi 工具库，可以：
- 创建和修改图形实体
- 访问图层、块表等符号表
- 实现用户交互
- 使用 KTArxTool 提供的实用函数

## 依赖项

- ZWCAD ZRX SDK
- MFC 库（通过 Visual Studio 提供）
- arxApi 项目（KTArxTool 工具库）

## 参考

- arxApi：基础 ARX API 封装库
- main：Qt + ARX 的另一个示例项目
