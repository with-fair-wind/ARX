#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Command/command.h>
#include <Services/zcBm_lisp_bridge.h>
#include <Utils/zcBm_resbuf_codec.h>
#include <acedads.h>
#include <acutads.h>


namespace {
using ZcBmLispHandler = ZcBmLispBridge::ZcBmLispHandler;
using ZcBmLispFunctionEntry = ZcBmLispBridge::ZcBmLispFunctionEntry;

template <typename T>
bool parseLispArg(const resbuf* node, T* out_value) {
    if (node == nullptr) {
        return false;
    }
    return ZcBmResbufCodec<T>::decode(node, static_cast<short>(node->restype), out_value);
}

template <typename... Args>
using DecayedArgsTuple = std::tuple<typename std::decay<Args>::type...>;

bool hasNoMoreArgs(const resbuf* node) {
    return node == nullptr || (node->restype == RTNONE && node->rbnext == nullptr);
}

template <std::size_t Index, typename Tuple>
bool parseTupleElement(const resbuf*& current, Tuple* out_values) {
    if (current == nullptr || out_values == nullptr) {
        return false;
    }
    typedef typename std::tuple_element<Index, Tuple>::type ArgT;
    ArgT value{};
    if (!parseLispArg<ArgT>(current, &value)) {
        return false;
    }
    std::get<Index>(*out_values) = std::move(value);
    current = current->rbnext;
    return true;
}

template <typename Tuple, std::size_t... Indices>
bool parseTypedArgsImpl(const resbuf* args, Tuple* out_values, std::index_sequence<Indices...>) {
    if (out_values == nullptr) {
        return false;
    }

    const resbuf* current = args;
    bool ok = true;
    std::initializer_list<int>{(ok = ok && parseTupleElement<Indices, Tuple>(current, out_values), 0)...};
    if (!ok) {
        return false;
    }
    return hasNoMoreArgs(current);
}

template <typename... Args>
bool parseTypedArgs(const resbuf* args, DecayedArgsTuple<Args...>* out_values) {
    return parseTypedArgsImpl(args, out_values, std::make_index_sequence<sizeof...(Args)>{});
}

template <typename Fn, typename Tuple, std::size_t... Indices>
auto invokeFromTupleImpl(Fn&& fn, Tuple& values, std::index_sequence<Indices...>) -> decltype(fn(std::move(std::get<Indices>(values))...)) {
    return std::forward<Fn>(fn)(std::move(std::get<Indices>(values))...);
}

template <typename Fn, typename Tuple>
auto invokeFromTuple(Fn&& fn, Tuple& values) -> decltype(invokeFromTupleImpl(std::forward<Fn>(fn), values, std::make_index_sequence<std::tuple_size<Tuple>::value>{})) {
    return invokeFromTupleImpl(std::forward<Fn>(fn), values, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

void printWStringAddress(const ZTCHAR* prefix, const std::wstring& value) {
    acutPrintf(_T("\n%s addr=%p, size=%d, text=%ls"), prefix, static_cast<const void*>(&value), static_cast<int>(value.size()), value.c_str());
}

template <typename Callback>
ZcBmLispHandler makeWStringCategoryHandler(const ZTCHAR* function_name, const ZTCHAR* title, Callback callback) {
    typedef typename std::decay<Callback>::type DecayedCallback;
    return [name = std::wstring(function_name), case_title = std::wstring(title), cb = DecayedCallback(callback)](resbuf* args) -> int {
        DecayedArgsTuple<std::wstring> values;
        if (!parseTypedArgs<std::wstring>(args, &values)) {
            acutPrintf(_T("\n[LISP:%ls] 参数不匹配。"), name.c_str());
            zcedRetNil();
            return RTERROR;
        }

        acutPrintf(_T("\n---- callback case: %ls ----"), case_title.c_str());
        printWStringAddress(_T("[handler before] tuple"), std::get<0>(values));
        invokeFromTuple(cb, values);
        printWStringAddress(_T("[handler after ] tuple"), std::get<0>(values));
        zcedRetStr(case_title.c_str());
        return RTNORM;
    };
}

template <typename... Args, typename Fn>
ZcBmLispHandler makeTypedLispHandler(const ZTCHAR* function_name, Fn fn) {
    typedef typename std::decay<Fn>::type DecayedFn;
    return [name = std::wstring(function_name), fn_capture = DecayedFn(fn)](resbuf* args) -> int {
        DecayedArgsTuple<Args...> values;
        if (!parseTypedArgs<Args...>(args, &values)) {
            acutPrintf(_T("\n[LISP:%ls] 参数不匹配。"), name.c_str());
            zcedRetNil();
            return RTERROR;
        }
        return invokeFromTuple(fn_capture, values);
    };
}

ZcBmLispBridge::ZcBmLispCache makeInitialLispCache() {
    return ZcBmLispBridge::ZcBmLispCache{std::vector<ZcBmLispFunctionEntry>{
        {L"mfcEchoNoArgs", makeTypedLispHandler<>(_T("mfcEchoNoArgs"),
                                                  []() -> int {
                                                      acutPrintf(_T("\n[LISP] mfcEchoNoArgs 被调用。"));
                                                      zcedRetStr(_T("MFC Lisp NoArgs OK"));
                                                      return RTNORM;
                                                  })},
        {L"mfcSumIntReal", makeTypedLispHandler<int, double>(_T("mfcSumIntReal"),
                                                             [](int left, double right) -> int {
                                                                 const double result = static_cast<double>(left) + right;
                                                                 acutPrintf(_T("\n[LISP] mfcSumIntReal 参数: %d, %.6f => %.6f"), left, right, result);
                                                                 zcedRetReal(result);
                                                                 return RTNORM;
                                                             })},
        {L"mfcDescribePoint", makeTypedLispHandler<AcGePoint3d, std::wstring>(_T("mfcDescribePoint"),
                                                                              [](const AcGePoint3d& pt, const std::wstring& label) -> int {
                                                                                  CString text;
                                                                                  text.Format(_T("%ls:(%.3f, %.3f, %.3f)"), label.c_str(), pt.x, pt.y, pt.z);
                                                                                  acutPrintf(_T("\n[LISP] mfcDescribePoint => %s"), text.GetString());
                                                                                  zcedRetStr(text.GetString());
                                                                                  return RTNORM;
                                                                              })},
        {L"mfcCatByValue", makeWStringCategoryHandler(_T("mfcCatByValue"), _T("by value"), [](std::wstring text) { printWStringAddress(_T("[callback value ]"), text); })},
        {L"mfcCatByConstLRef", makeWStringCategoryHandler(_T("mfcCatByConstLRef"), _T("by const lref"), [](const std::wstring& text) { printWStringAddress(_T("[callback const&]"), text); })},
        {L"mfcCatByRRef", makeWStringCategoryHandler(_T("mfcCatByRRef"), _T("by rref"), [](std::wstring&& text) { printWStringAddress(_T("[callback &&    ]"), text); })},
        {L"mfcCatByConstRRef", makeWStringCategoryHandler(_T("mfcCatByConstRRef"), _T("by const rref"), [](const std::wstring&& text) { printWStringAddress(_T("[callback const&&]"), text); })},
    }};
}

const ZcBmLispBridge::ZcBmLispCache& initialLispCache() {
    static const ZcBmLispBridge::ZcBmLispCache cache = makeInitialLispCache();
    return cache;
}

ZcBmLispBridge::ZcBmLispCache& defaultLispCache() {
    static ZcBmLispBridge::ZcBmLispCache cache = makeInitialLispCache();
    return cache;
}

void resetDefaultLispCacheToInitial() {
    defaultLispCache() = initialLispCache();
}

int dynamicEchoRawHandler(resbuf* args) {
    std::wstring payload;
    if (args == nullptr) {
        acutPrintf(_T("\n[LISP-DYN] 无参数调用。"));
        zcedRetStr(_T("DYN:NO_ARGS"));
        return RTNORM;
    }

    if (!parseLispArg<std::wstring>(args, &payload)) {
        acutPrintf(_T("\n[LISP-DYN] 参数类型不匹配，期望字符串。"));
        zcedRetNil();
        return RTERROR;
    }

    CString result;
    result.Format(_T("DYN:ECHO:%ls"), payload.c_str());
    acutPrintf(_T("\n[LISP-DYN] 收到参数: %ls"), payload.c_str());
    zcedRetStr(result.GetString());
    return RTNORM;
}

template <typename... Args>
int invokeLispFunction(const ZcString& function_name, Args&&... args) {
    ZcBmResbufChain chain;
    if (!ZcBmResbufCodec<ZcString>::encode(RTSTR, function_name, &chain)) {
        acutPrintf(_T("\n[zcedInvoke] 构建函数名失败: %s"), function_name.kTCharPtr());
        chain.release();
        return RTERROR;
    }

    bool args_ok = true;
    std::initializer_list<int>{
        (args_ok = args_ok && ZcBmResbufCodec<typename std::decay<Args>::type>::encode(ZcBmLispBridge::ZcBmLispInvokeRestype<typename std::decay<Args>::type>::value, std::forward<Args>(args), &chain),
         0)...};
    if (!args_ok) {
        acutPrintf(_T("\n[zcedInvoke] 构建参数失败: %s"), function_name.kTCharPtr());
        chain.release();
        return RTERROR;
    }
    if (!chain.appendTerminator()) {
        acutPrintf(_T("\n[zcedInvoke] 构建 RTNONE 终止参数失败: %s"), function_name.kTCharPtr());
        chain.release();
        return RTERROR;
    }

    resbuf* result = nullptr;
    const int status = zcedInvoke(chain.head(), &result);
    chain.release();

    if (status != RTNORM) {
        acutPrintf(_T("\n[zcedInvoke] 调用失败: %s, status=%d"), function_name.kTCharPtr(), status);
        if (result != nullptr) {
            acutRelRb(result);
        }
        return status;
    }

    if (result != nullptr) {
        switch (result->restype) {
            case RTREAL:
                acutPrintf(_T("\n[zcedInvoke] 返回值(REAL)=%.6f"), result->resval.rreal);
                break;
            case RTSHORT:
                acutPrintf(_T("\n[zcedInvoke] 返回值(SHORT)=%d"), result->resval.rint);
                break;
            case RTLONG:
                acutPrintf(_T("\n[zcedInvoke] 返回值(LONG)=%d"), result->resval.rlong);
                break;
            case RTSTR:
                acutPrintf(_T("\n[zcedInvoke] 返回值(STR)=%s"), result->resval.rstring == nullptr ? _T("") : result->resval.rstring);
                break;
            case RTNIL:
                acutPrintf(_T("\n[zcedInvoke] 返回值=NIL"));
                break;
            default:
                acutPrintf(_T("\n[zcedInvoke] 返回值类型=%d"), result->restype);
                break;
        }
        acutRelRb(result);
    } else {
        acutPrintf(_T("\n[zcedInvoke] 无返回值。"));
    }
    return RTNORM;
}
}  // namespace

namespace ZcBmLispBridge {
int ZcBmRegisterLispCommands(ZcBmLispCache& cache) {
    const auto& table = cache.m_table;
    for (size_t i = 0; i < table.size(); ++i) {
        if (table[i].handler == nullptr || table[i].function_name.isEmpty()) {
            continue;
        }
        const int func_code = kZcBmLispFuncCodeBase + static_cast<int>(i);
        if (zcedDefun(table[i].function_name.kTCharPtr(), func_code) == 0) {
            acutPrintf(_T("\n[LISP] 注册失败: %s"), table[i].function_name.kTCharPtr());
            return RTERROR;
        }
    }

    acutPrintf(_T("\n[LISP] 已注册 %d 个函数。"), static_cast<int>(table.size()));
    return RTNORM;
}

int ZcBmUnregisterLispCommands(ZcBmLispCache& cache) {
    const auto& table = cache.m_table;
    for (size_t i = 0; i < table.size(); ++i) {
        if (table[i].handler == nullptr || table[i].function_name.isEmpty()) {
            continue;
        }
        const int func_code = kZcBmLispFuncCodeBase + static_cast<int>(i);
        zcedUndef(table[i].function_name.kTCharPtr(), func_code);
    }
    return RTNORM;
}

int ZcBmDispatchLispCommand(ZcBmLispCache& cache) {
    const auto& table = cache.m_table;
    const int function_code = zcedGetFunCode();
    const int local_index = function_code - kZcBmLispFuncCodeBase;
    if (local_index < 0 || local_index >= static_cast<int>(table.size())) {
        return kZcBmLispDispatchNotHandled;
    }
    if (table[static_cast<size_t>(local_index)].handler == nullptr) {
        acutPrintf(_T("\n[LISP] 空处理器编码: %d"), function_code);
        zcedRetNil();
        return RTERROR;
    }

    resbuf* args = zcedGetArgs();
    const int result = table[static_cast<size_t>(local_index)].handler(args);
    if (args != nullptr) {
        acutRelRb(args);
    }
    return result;
}

int invokeLispFunction(resbuf* args, resbuf** out_result) {
    if (args == nullptr) {
        return RTERROR;
    }

    resbuf* result = nullptr;
    const int status = zcedInvoke(args, &result);
    acutRelRb(args);
    if (out_result != nullptr) {
        *out_result = result;
    } else if (result != nullptr) {
        acutRelRb(result);
    }
    return status;
}

int ZcBmAddLispFunction(ZcBmLispCache& cache, const ZcString& function_name, const ZcBmLispHandler& handler, int* out_func_code) {
    if (function_name.isEmpty() || handler == nullptr) {
        return RTERROR;
    }

    size_t target_index = 0;
    ZcBmLispHandler previous_handler;
    if (cache.push(function_name, handler, &target_index, &previous_handler) != RTNORM) {
        return RTERROR;
    }

    const int func_code = kZcBmLispFuncCodeBase + static_cast<int>(target_index);
    if (zcedDefun(function_name.kTCharPtr(), func_code) == 0) {
        if (previous_handler != nullptr) {
            size_t rollback_index = 0;
            ZcBmLispHandler ignored_previous;
            if (cache.push(function_name, previous_handler, &rollback_index, &ignored_previous) != RTNORM || rollback_index != target_index) {
                return RTERROR;
            }
        } else {
            cache.remove(function_name, nullptr, nullptr, nullptr);
        }
        return RTERROR;
    }

    if (out_func_code != nullptr) {
        *out_func_code = func_code;
    }
    return RTNORM;
}

int ZcBmRemoveLispFunction(ZcBmLispCache& cache, const ZcString& function_name) {
    if (function_name.isEmpty()) {
        return RTERROR;
    }

    size_t index = 0;
    ZcString registered_name;
    ZcBmLispHandler removed_handler;
    if (cache.remove(function_name, &index, &registered_name, &removed_handler) != RTNORM) {
        return RTERROR;
    }

    const int func_code = kZcBmLispFuncCodeBase + static_cast<int>(index);
    if (zcedUndef(registered_name.kTCharPtr(), func_code) == 0) {
        size_t rollback_index = 0;
        ZcBmLispHandler ignored_previous;
        if (cache.push(registered_name, removed_handler, &rollback_index, &ignored_previous) != RTNORM || rollback_index != index) {
            return RTERROR;
        }
        return RTERROR;
    }
    return RTNORM;
}
}  // namespace ZcBmLispBridge

int ZcBmRegisterLispCommands() {
    resetDefaultLispCacheToInitial();
    return ZcBmLispBridge::ZcBmRegisterLispCommands(defaultLispCache());
}
int ZcBmUnregisterLispCommands() {
    return ZcBmLispBridge::ZcBmUnregisterLispCommands(defaultLispCache());
}
int ZcBmDispatchLispCommand() {
    return ZcBmLispBridge::ZcBmDispatchLispCommand(defaultLispCache());
}

void MfcLispInvokeNoArgsCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行 zcedInvoke 测试：无参数"));
    invokeLispFunction(L"mfcEchoNoArgs");
}

void MfcLispInvokeIntRealCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行 zcedInvoke 测试：int + real"));
    invokeLispFunction(L"mfcSumIntReal", 42, 3.5);
}

void MfcLispInvokePointStringCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行 zcedInvoke 测试：point + string"));
    invokeLispFunction(L"mfcDescribePoint", AcGePoint3d(12.0, 34.5, 0.0), std::wstring(L"测试点"));
}

void MfcLispValueCategoryTestCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行值类别回调验证（zcedInvoke -> kInvkSubrMsg -> handler）..."));

    invokeLispFunction(L"mfcCatByValue", std::wstring(L"value_case"));
    invokeLispFunction(L"mfcCatByConstLRef", std::wstring(L"const_lref_case"));
    invokeLispFunction(L"mfcCatByRRef", std::wstring(L"rref_case"));
    invokeLispFunction(L"mfcCatByConstRRef", std::wstring(L"const_rref_case"));

    acutPrintf(_T("\n说明: 以上日志来自真实回调链路；当前 invokeFromTuple 使用 std::move 展开。"));
}

void MfcLispDynamicApiTestCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行动态 Lisp API 测试（Add -> Invoke -> Remove -> Invoke）..."));

    const ZTCHAR* dynamic_name = _T("mfcDynEcho");
    ZcBmRemoveLispFunction(dynamic_name);

    int func_code = -1;
    const int add_status = ZcBmAddLispFunction(dynamic_name, dynamicEchoRawHandler, &func_code);
    if (add_status != RTNORM) {
        acutPrintf(_T("\n[LISP-DYN] Add 失败，status=%d"), add_status);
        return;
    }
    acutPrintf(_T("\n[LISP-DYN] Add 成功，funcCode=%d"), func_code);

    invokeLispFunction(L"mfcDynEcho", std::wstring(L"hello_dynamic"));

    const int remove_status = ZcBmRemoveLispFunction(dynamic_name);
    acutPrintf(_T("\n[LISP-DYN] Remove status=%d"), remove_status);

    const int second_invoke_status = invokeLispFunction(L"mfcDynEcho", std::wstring(L"should_fail"));
    acutPrintf(_T("\n[LISP-DYN] Remove 后调用 status=%d（预期失败）"), second_invoke_status);
}

int MfcRegisterLispCommands() {
    return ZcBmRegisterLispCommands();
}
int MfcUnregisterLispCommands() {
    return ZcBmUnregisterLispCommands();
}
int MfcDispatchLispCommand() {
    return ZcBmDispatchLispCommand();
}

int ZcBmAddLispFunction(const ZTCHAR* function_name, const ZcBmLispRawHandler& handler, int* out_func_code) {
    if (function_name == nullptr || *function_name == 0 || handler == nullptr) {
        return RTERROR;
    }
    return ZcBmLispBridge::ZcBmAddLispFunction(defaultLispCache(), ZcString(function_name), ZcBmLispBridge::ZcBmLispHandler(handler), out_func_code);
}

int ZcBmRemoveLispFunction(const ZTCHAR* function_name) {
    if (function_name == nullptr || *function_name == 0) {
        return RTERROR;
    }
    return ZcBmLispBridge::ZcBmRemoveLispFunction(defaultLispCache(), ZcString(function_name));
}

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