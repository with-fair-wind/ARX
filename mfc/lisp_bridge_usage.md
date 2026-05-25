# ZcBmLispBridge Lisp 命令注册调用框架使用指南

---

## 前言：需求说明

当前族模板管理中，针对建筑的族类型，右键布置功能未适配。

适配方案如下：

- 建筑开发需要实现“针对建筑族类型的布置命令”，并将该命令注册为 Lisp 命令。
- 族模板管理器模块（建筑开发无需关注内部实现）会动态调用布置策略；当族类型判断为建筑族类型时，调用对应的 Lisp 命令执行布置。
- 建筑开发者可使用本框架完成 Lisp 命令定义与注册。目前墙和柱已完成适配并验证可用。

针对该需求，布置命令签名建议为：

```cpp
int (*)(const ZcDbObjectId& idTemplateType)
```

注册方式可参考本文使用示例，具体实现可对照 `ZAecWall.cpp`。

目前已预定义的建筑类别与 Lisp 命令名称对应关系如下：

| 建筑类别 | Lisp 命令名 | 状态 |
| --- | --- | --- |
| 墙 | `BmPlaceWall` | 已启用 |
| 柱 | `BmPlaceColumn` | 已启用 |
| 门窗 | `BmPlaceDoorWindow` | 已启用 |
| 墙体洞口 | `BmPlaceWallOpening` | 预留（当前注释） |
| 洞口 | `BmPlaceHole` | 已启用 |
| 竖井 | `BmPlaceShaft` | 已启用 |
| 屋顶 | `BmPlaceRoof` | 已启用 |
| 楼梯 | `BmPlaceStair` | 已启用 |
| 坡道 | `BmPlaceRamp` | 已启用 |
| 散水 | `BmPlaceApron` | 已启用 |
| 台阶 | `BmPlaceStep` | 已启用 |
| 电梯 | `BmPlaceElevator` | 已启用 |
| 扶梯 | `BmPlaceEscalator` | 已启用 |
| 栏杆扶手 | `BmPlaceRailing` | 已启用 |
| 楼板 | `BmPlaceFloor` | 已启用 |
| 洁具 | `BmPlaceSanitaryFixture` | 已启用 |
| 房间 | `BmPlaceSpace` | 已启用 |
| 阳台 | `BmPlaceBalcony` | 已启用 |
| 梁 | `BmPlaceBeam` | 已启用 |
| 车位 | `BmPlaceParkingSpace` | 已启用 |

本框架为通用框架，凡涉及 Lisp 命令注册与调用的相关需求，均可优先考虑使用该框架实现。核心优势是支持任意参数，并封装了调用方的 `resbuf` 构造与命令实现方的参数解析逻辑，减少重复样板代码和接入成本。

---

## 1. 目标与能力

### 1.1 核心目标

`ZcBmLispBridge` 负责把 Lisp 命令的“注册 / 分发 / 反注册 / 调用”统一到一套 API。  
业务函数不再手工拼接和解析大量 `resbuf` 细节，维护成本更低。

框架覆盖能力：

- 命令注册：`zcedDefun`
- 命令分发：`On_kInvkSubrMsg` 中按 `funcCode` 分发
- 命令反注册：`zcedUndef`
- 主动调用 Lisp：`zcedInvoke`

### 1.2 两种处理模式

- **Raw 模式**：`int(resbuf*)`，兼容历史变参接口。
- **Typed 模式**：`ZcBmAddLispFunction<Args...>(...)`，按模板参数自动解码 `resbuf`，函数签名更清晰。

### 1.3 “支持任意参数”的边界

“任意参数”指的是参数个数可变（`Args...`），但每个参数类型需满足框架当前实现的类型约束

### 1.4 类型支持

| 类型 | 是否可用于 typed invoke |
| --- | --- |
| `int`（及等价 `std::int32_t`） | 是 |
| `std::int16_t` | 是 |
| `std::int64_t` | 是 |
| `bool` | 是 |
| `double` | 是 |
| `ZcString` | 是 |
| `ZTCHAR*` / `const ZTCHAR*` | 是 |
| `ZcGePoint3d` | 是 |
| `zds_name` | 是 |
| `ZcDbObjectId` | 是 |

---

## 2. 数据结构与 API

### 2.1 数据结构

```cpp

```

说明：

- `m_table` 是命令槽位表，`funcCode = m_lispFuncCodeBase + index`。
- `m_lispFuncCodeBase` 在缓存级别配置，用作桥接框架内部命令编号的起始偏移，主要用于同一 ZRX 模块内的 Lisp 命令编码空间隔离，兼顾分发阶段的快速未命中判定。
- `clear()` 仅清空表，不自动做 `undef`；通常先 `Unregister` 再清缓存。

### 2.2 接口总览（按图中签名）

| 接口 | 功能 | 说明 |
| --- | --- | --- |
| `int ZcBmRegisterLispCommands(const ZcBmLispCache& cache)` | 批量注册缓存命令 | 只读 cache，逐项 `zcedDefun` |
| `int ZcBmUnregisterLispCommands(const ZcBmLispCache& cache)` | 批量反注册缓存命令 | 只读 cache，逐项 `zcedUndef` |
| `int ZcBmDispatchLispCommand(const ZcBmLispCache& cache)` | 按 funcCode 分发 | 未命中返回 `kZcBmLispDispatchNotHandled` |
| `int ZcBmAddLispFunction(ZcBmLispCache& cache, const ZcString& function_name, const ZcBmLispHandler& handler, int* out_func_code = nullptr)` | Raw 动态注册 | 可覆盖同名命令处理器 |
| `template <typename... Args, typename Fn> int ZcBmAddLispFunction(ZcBmLispCache& cache, const ZcString& function_name, Fn&& fn, int* out_func_code = nullptr)` | Typed 动态注册 | 需显式给出模板参数类型（无参可写 `<>` 或省略） |
| `int ZcBmRemoveLispFunction(ZcBmLispCache& cache, const ZcString& function_name)` | 动态移除命令 | 置空 `m_handler` 并执行 `zcedUndef` |
| `int invokeLispFunction(resbuf* args, resbuf** o_result = nullptr)` | 直接 `resbuf` 调用 Lisp | 兼容旧调用路径 |
| `template <typename... Args> int invokeLispFunction(const ZcString& function_name, resbuf** o_result, Args&&... args)` | Typed 调用 Lisp | 内部编码参数并调用上面的 raw invoke |

### 2.3 常量与返回码语义

- `kZcBmLispDispatchNotHandled = -1`：仅表示“不是桥接框架命令”，不是执行失败。
- `RTNORM`：成功。
- `RTERROR`：命中桥接流程但处理失败（如参数不匹配、`defun/undef/invoke` 失败）。

---

## 3. 使用步骤（先总体，再示例）

### 3.1 总体流程

1. 定义全局缓存：`ZcBmLispCache g_lispCache;/ZcBmLispCache g_lispCache(2000);`
2. `On_kLoadDwgMsg`：`Add`（可多次）
3. `On_kInvkSubrMsg`：调用 `Dispatch`。
4. `On_kUnloadDwgMsg`：调用 `Unregister`。
5. 运行期动态维护：`Add` / `Remove`。

### 3.2 示例 A：Zrx模块全局缓存定义、注册、反注册、分发

```cpp
ZcBmLispBridge::ZcBmLispCache g_lispCache;
/* 
    或根据当前zrx已经注册的lisp命令列表 指定 m_lispFuncCodeBase 创建
    eg. ZAecBlockAndPattern.cpp中已经注册了 off_1C5920E0 列表的 lisp 命令
    则应用此框架需将 m_lispFuncCodeBase 指定为 >= off_1C5920E0.size() 的值
    ZcBmLispBridge::ZcBmLispCache g_lispCache(>= off_1C5920E0.size())
*/

void RegisterLispForCurrentDwg() {
    // Typed 注册（推荐）
    ZcBmLispBridge::ZcBmAddLispFunction<>(g_lispCache, ZW_T("BmNoArg"), BmNoArg);
    ZcBmLispBridge::ZcBmAddLispFunction<ZcDbObjectId>(g_lispCache, ZW_T("BmPlaceWall"), ZABmPlaceWall);
    ZcBmLispBridge::ZcBmAddLispFunction<int, double>(g_lispCache, ZW_T("BmCalc"), BmCalc);

    // Raw 注册（旧式兼容）
    ZcBmLispBridge::ZcBmAddLispFunction(g_lispCache, ZW_T("BmLegacy"), BmLegacyRaw);
}

virtual ZcRx::AppRetCode On_kLoadDwgMsg(void* pkt) {
    const auto ret = ZcRxZrxApp::On_kLoadDwgMsg(pkt);
    RegisterLispForCurrentDwg();
    return ret;
}

virtual ZcRx::AppRetCode On_kInvkSubrMsg(void* pkt) {
    const auto ret = ZcRxZrxApp::On_kInvkSubrMsg(pkt);
    const int status = ZcBmLispBridge::ZcBmDispatchLispCommand(g_lispCache);
    if (status == ZcBmLispBridge::kZcBmLispDispatchNotHandled) {
        // 保留外层原有分发链路
    }
    return ret;
}

virtual ZcRx::AppRetCode On_kUnloadDwgMsg(void* pkt) {
    const auto ret = ZcRxZrxApp::On_kUnloadDwgMsg(pkt);
    ZcBmLispBridge::ZcBmUnregisterLispCommands(g_lispCache);
    return ret;
}
```

### 3.3 示例 B：任意参数Lisp命令定义及动态注册

```cpp
int ZcBmPlaceWall(const ZcDbObjectId& wallTypeId) {
    if (wallTypeId.isNull()) {
        zcedRetNil();
        return RTERROR;
    }
    zcedRetStr(ZW_T("BmPlaceWall OK"));
    return RTNORM;
}

int ZcBmCalc(int left, double right) {
    const double result = static_cast<double>(left) + right;
    acutPrintf(_T("\n[LISP] mfcSumIntReal 参数: %d, %.6f => %.6f"), left, right, result);
    zcedRetReal(result);
    return RTNORM;
}

ZcBmLispBridge::ZcBmAddLispFunction<ZcDbObjectId>(g_lispCache, ZW_T("BmPlaceWall"), ZcBmPlaceWall);
ZcBmLispBridge::ZcBmAddLispFunction<ZcDbObjectId>(g_lispCache, ZW_T("BmCalc"), ZcBmCalc);
```

### 3.4 示例 C：兼容旧式处理resbuf方式的命令定义及动态注册

```cpp
int ZcBmLegacyRaw(resbuf* rb) {
    resbuf* cur = rb;
    if (cur->restype == RTREAL) {
        double data = cur->resval.rreal;
        zcutPrintf(T("\n[LIsP] lisp Testcmd5 参数1:%.6f"), data);
        cur = cur->rbnext;
    }
    if (cur->restype == RTSHORT) {
        short data = cur->resval.rint;
        zcutPrintf(T("\n[LISP] lisp Testcmd5 参数2:%d"), data);
    }
    return RTNORM;
}

ZcBmLispBridge::ZcBmAddLispFunction(g_lispCache, ZW_T("BmLegacy"), BmLegacyRaw);
```

### 3.5 示例 D：调用 Lisp命令方式

```cpp
// Typed invoke，结果通过 o_result 传出（不需要可传 nullptr）
resbuf* result = nullptr;
const int invoke_status = ZcBmLispBridge::invokeLispFunction(ZW_T("BmCalc"), &result, 42, 3.1415926);
if (result != nullptr) {
    zcutRelRb(result);
}

// Raw invoke：自行构造参数链
// resbuf invoke（用于 typed invoke 暂不支持的参数形态）
ZcEntRbList rblist(zutBuildList(RTSTR, ZCRX_T("BmLegacy"), RTREAL, 13.14, RTSHORT, 2, 0));
resbuf* result = nullptr;
ZcBmLispBridge::invokeLispFunction(rblist, &result);
if (result != nullptr) {
    zcutRelRb(result);
}
```

### 3.6 示例 E：动态新增/删除

```cpp
int func_code = -1;
const int add_status = ZcBmLispBridge::ZcBmAddLispFunction(g_lispCache, ZW_T("MyRuntimeCmd"), MyRawHandler, &func_code);

const int remove_status = ZcBmLispBridge::ZcBmRemoveLispFunction(g_lispCache, ZW_T("MyRuntimeCmd"));
```

---

## 4. 行为注意点

- `Add` 同名时会复用原 index，并重新 `defun`，默认是“覆盖处理器”语义。
- `Remove` 成功后将对应槽位 `m_handler = nullptr`，不压缩 `m_table`，保证既有 `funcCode` 映射稳定。
- `Register/Unregister/Dispatch` 接收 `const ZcBmLispCache&`，不会修改缓存结构本身。
- `invokeLispFunction(..., resbuf** o_result)` 传出结果后，调用方负责释放 `*o_result`（`zcutRelRb`）。
