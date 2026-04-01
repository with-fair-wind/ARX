#include <Command/command.h>
#include <Services/zcBm_lisp_bridge.h>
#include <Utils/zcBm_resbuf_codec.h>
#include <acedads.h>
#include <acutads.h>

#include <functional>
#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace {
using ZcBmLispHandler = std::function<int(resbuf*)>;

struct ZcBmLispFunctionEntry {
    std::wstring function_name;
    ZcBmLispHandler handler;
};

template <typename T>
struct ZcBmLispInvokeRestype;

template <>
struct ZcBmLispInvokeRestype<int> {
    static const int value = RTLONG;
};
template <>
struct ZcBmLispInvokeRestype<std::int16_t> {
    static const int value = RTSHORT;
};
template <>
struct ZcBmLispInvokeRestype<std::int64_t> {
    static const int value = RTINT64;
};
template <>
struct ZcBmLispInvokeRestype<double> {
    static const int value = RTREAL;
};
template <>
struct ZcBmLispInvokeRestype<std::wstring> {
    static const int value = RTSTR;
};
template <>
struct ZcBmLispInvokeRestype<AcGePoint3d> {
    static const int value = RT3DPOINT;
};
template <>
struct ZcBmLispInvokeRestype<zds_name> {
    static const int value = RTENAME;
};

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
auto invokeFromTuple(Fn&& fn, Tuple& values)
    -> decltype(invokeFromTupleImpl(std::forward<Fn>(fn), values, std::make_index_sequence<std::tuple_size<Tuple>::value>{})) {
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
    return [name = std::wstring(function_name), callback = DecayedFn(fn)](resbuf* args) -> int {
        DecayedArgsTuple<Args...> values;
        if (!parseTypedArgs<Args...>(args, &values)) {
            acutPrintf(_T("\n[LISP:%ls] 参数不匹配。"), name.c_str());
            zcedRetNil();
            return RTERROR;
        }
        return invokeFromTuple(callback, values);
    };
}

std::vector<ZcBmLispFunctionEntry>& lispFunctionTable() {
    static std::vector<ZcBmLispFunctionEntry> table = {
        {L"mfcEchoNoArgs",
         makeTypedLispHandler<>(_T("mfcEchoNoArgs"), []() -> int {
             acutPrintf(_T("\n[LISP] mfcEchoNoArgs 被调用。"));
             zcedRetStr(_T("MFC Lisp NoArgs OK"));
             return RTNORM;
         })},
        {L"mfcSumIntReal",
         makeTypedLispHandler<int, double>(_T("mfcSumIntReal"), [](int left, double right) -> int {
             const double result = static_cast<double>(left) + right;
             acutPrintf(_T("\n[LISP] mfcSumIntReal 参数: %d, %.6f => %.6f"), left, right, result);
             zcedRetReal(result);
             return RTNORM;
         })},
        {L"mfcDescribePoint",
         makeTypedLispHandler<AcGePoint3d, std::wstring>(_T("mfcDescribePoint"), [](const AcGePoint3d& pt, const std::wstring& label) -> int {
             CString text;
             text.Format(_T("%ls:(%.3f, %.3f, %.3f)"), label.c_str(), pt.x, pt.y, pt.z);
             acutPrintf(_T("\n[LISP] mfcDescribePoint => %s"), text.GetString());
             zcedRetStr(text.GetString());
             return RTNORM;
         })},
        {L"mfcCatByValue",
         makeWStringCategoryHandler(_T("mfcCatByValue"), _T("by value"), [](std::wstring text) {
             printWStringAddress(_T("[callback value ]"), text);
         })},
        {L"mfcCatByConstLRef",
         makeWStringCategoryHandler(_T("mfcCatByConstLRef"), _T("by const lref"), [](const std::wstring& text) {
             printWStringAddress(_T("[callback const&]"), text);
         })},
        {L"mfcCatByRRef",
         makeWStringCategoryHandler(_T("mfcCatByRRef"), _T("by rref"), [](std::wstring&& text) {
             printWStringAddress(_T("[callback &&    ]"), text);
         })},
        {L"mfcCatByConstRRef",
         makeWStringCategoryHandler(_T("mfcCatByConstRRef"), _T("by const rref"), [](const std::wstring&& text) {
             printWStringAddress(_T("[callback const&&]"), text);
         })},
    };
    return table;
}

bool& lispRegistrationState() {
    static bool registered = false;
    return registered;
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
int invokeLispFunction(const std::wstring& function_name, Args&&... args) {
    ZcBmResbufChain chain;
    if (!ZcBmResbufCodec<std::wstring>::encode(RTSTR, function_name, &chain)) {
        acutPrintf(_T("\n[zcedInvoke] 构建函数名失败: %ls"), function_name.c_str());
        chain.release();
        return RTERROR;
    }

    bool args_ok = true;
    std::initializer_list<int>{(args_ok = args_ok &&
                                           ZcBmResbufCodec<typename std::decay<Args>::type>::encode(
                                               ZcBmLispInvokeRestype<typename std::decay<Args>::type>::value, std::forward<Args>(args), &chain),
                                0)...};
    if (!args_ok) {
        acutPrintf(_T("\n[zcedInvoke] 构建参数失败: %ls"), function_name.c_str());
        chain.release();
        return RTERROR;
    }
    if (!chain.appendTerminator()) {
        acutPrintf(_T("\n[zcedInvoke] 构建 RTNONE 终止参数失败: %ls"), function_name.c_str());
        chain.release();
        return RTERROR;
    }

    resbuf* result = nullptr;
    const int status = zcedInvoke(chain.head(), &result);
    chain.release();

    if (status != RTNORM) {
        acutPrintf(_T("\n[zcedInvoke] 调用失败: %ls, status=%d"), function_name.c_str(), status);
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

int ZcBmRegisterLispCommands() {
    if (lispRegistrationState()) {
        return RTNORM;
    }

    const auto& table = lispFunctionTable();
    for (size_t i = 0; i < table.size(); ++i) {
        if (table[i].handler == nullptr || table[i].function_name.empty()) {
            continue;
        }
        if (zcedDefun(table[i].function_name.c_str(), static_cast<int>(i)) == 0) {
            acutPrintf(_T("\n[LISP] 注册失败: %ls"), table[i].function_name.c_str());
            return RTERROR;
        }
    }

    acutPrintf(_T("\n[LISP] 已注册 %d 个函数。"), static_cast<int>(table.size()));
    lispRegistrationState() = true;
    return RTNORM;
}

int ZcBmUnregisterLispCommands() {
    if (!lispRegistrationState()) {
        return RTNORM;
    }

    const auto& table = lispFunctionTable();
    for (size_t i = 0; i < table.size(); ++i) {
        if (table[i].handler == nullptr || table[i].function_name.empty()) {
            continue;
        }
        if (zcedUndef(table[i].function_name.c_str(), static_cast<int>(i)) == 0) {
            acutPrintf(_T("\n[LISP] 反注册失败: %ls"), table[i].function_name.c_str());
            return RTERROR;
        }
    }

    lispRegistrationState() = false;
    return RTNORM;
}

int ZcBmDispatchLispCommand() {
    const auto& table = lispFunctionTable();
    const int function_code = zcedGetFunCode();
    if (function_code < 0 || function_code >= static_cast<int>(table.size()) || table[function_code].handler == nullptr) {
        acutPrintf(_T("\n[LISP] 未知函数编码: %d"), function_code);
        zcedRetNil();
        return RTERROR;
    }

    resbuf* args = zcedGetArgs();
    const int result = table[function_code].handler(args);
    if (args != nullptr) {
        acutRelRb(args);
    }
    return result;
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

int MfcRegisterLispCommands() { return ZcBmRegisterLispCommands(); }
int MfcUnregisterLispCommands() { return ZcBmUnregisterLispCommands(); }
int MfcDispatchLispCommand() { return ZcBmDispatchLispCommand(); }

int ZcBmAddLispFunction(const ZTCHAR* function_name, ZcBmLispRawHandler handler, int* out_func_code) {
    if (function_name == nullptr || *function_name == 0 || handler == nullptr) {
        return RTERROR;
    }

    auto& table = lispFunctionTable();
    const std::wstring name(function_name);

    for (size_t i = 0; i < table.size(); ++i) {
        if (table[i].function_name == name) {
            if (table[i].handler != nullptr) {
                return RTERROR;
            }
            table[i].handler = handler;
            if (lispRegistrationState() && zcedDefun(table[i].function_name.c_str(), static_cast<int>(i)) == 0) {
                table[i].handler = nullptr;
                return RTERROR;
            }
            if (out_func_code != nullptr) {
                *out_func_code = static_cast<int>(i);
            }
            return RTNORM;
        }
    }

    ZcBmLispFunctionEntry entry;
    entry.function_name = name;
    entry.handler = handler;
    table.push_back(std::move(entry));
    const int func_code = static_cast<int>(table.size() - 1);

    if (lispRegistrationState() && zcedDefun(table.back().function_name.c_str(), func_code) == 0) {
        table.pop_back();
        return RTERROR;
    }
    if (out_func_code != nullptr) {
        *out_func_code = func_code;
    }
    return RTNORM;
}

int ZcBmRemoveLispFunction(const ZTCHAR* function_name) {
    if (function_name == nullptr || *function_name == 0) {
        return RTERROR;
    }

    auto& table = lispFunctionTable();
    const std::wstring name(function_name);
    for (size_t i = 0; i < table.size(); ++i) {
        if (table[i].function_name != name || table[i].handler == nullptr) {
            continue;
        }
        if (lispRegistrationState() && zcedUndef(table[i].function_name.c_str(), static_cast<int>(i)) == 0) {
            return RTERROR;
        }
        table[i].handler = nullptr;
        return RTNORM;
    }
    return RTERROR;
}
