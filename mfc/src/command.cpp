#include <Command/command.h>
#include <Dialog/arxui_dialog.h>
#include <Dialog/component_browser_dialog.h>
#include <Dialog/load_to_doc_dialog.h>
#include <Dialog/new_family_dialog.h>
#include <Dialog/overwrite_confirm_dialog.h>
#include <Dialog/property_permission_dialog.h>
#include <Dialog/rename_template_dialog.h>
#include <Dialog/test_dialog.h>
#include <Services/new_family_backend.h>
#include <acutads.h>
#include <acedads.h>
#include <aduiFileDialog.h>

#include "acdocman.h"

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


#if 0
using LispHandler = std::function<int(resbuf*)>;

struct LispFunctionEntry {
    const ZTCHAR* function_name = nullptr;
    LispHandler handler;
};

bool appendResbufNode(resbuf** head, resbuf** tail, resbuf* node);
bool appendStringNode(resbuf** head, resbuf** tail, const std::wstring& text);

template <typename T>
struct LispTypeCodec;

template <typename T>
bool parseLispArg(const resbuf* node, T* out_value);

template <>
struct LispTypeCodec<int> {
    static bool decode(const resbuf* node, int* out_value) {
        if (node == nullptr || out_value == nullptr) {
            return false;
        }
        if (node->restype == RTSHORT) {
            *out_value = node->resval.rint;
            return true;
        }
        if (node->restype == RTLONG) {
            *out_value = node->resval.rlong;
            return true;
        }
        return false;
    }

    static bool encode(int value, resbuf** head, resbuf** tail) {
        resbuf* node = acutNewRb(RTLONG);
        if (node == nullptr) {
            return false;
        }
        node->resval.rlong = value;
        return appendResbufNode(head, tail, node);
    }
};

template <>
struct LispTypeCodec<double> {
    static bool decode(const resbuf* node, double* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != RTREAL) {
            return false;
        }
        *out_value = node->resval.rreal;
        return true;
    }

    static bool encode(double value, resbuf** head, resbuf** tail) {
        resbuf* node = acutNewRb(RTREAL);
        if (node == nullptr) {
            return false;
        }
        node->resval.rreal = value;
        return appendResbufNode(head, tail, node);
    }
};

template <>
struct LispTypeCodec<std::wstring> {
    static bool decode(const resbuf* node, std::wstring* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != RTSTR) {
            return false;
        }
        const ZTCHAR* text = node->resval.rstring;
        *out_value = (text == nullptr) ? L"" : std::wstring(text);
        return true;
    }

    static bool encode(const std::wstring& value, resbuf** head, resbuf** tail) { return appendStringNode(head, tail, value); }
};

template <>
struct LispTypeCodec<AcGePoint3d> {
    static bool decode(const resbuf* node, AcGePoint3d* out_value) {
        if (node == nullptr || out_value == nullptr) {
            return false;
        }
        if (node->restype != RTPOINT && node->restype != RT3DPOINT) {
            return false;
        }
        *out_value = AcGePoint3d(node->resval.rpoint[X], node->resval.rpoint[Y], node->resval.rpoint[Z]);
        return true;
    }

    static bool encode(const AcGePoint3d& value, resbuf** head, resbuf** tail) {
        resbuf* node = acutNewRb(RT3DPOINT);
        if (node == nullptr) {
            return false;
        }
        node->resval.rpoint[X] = value.x;
        node->resval.rpoint[Y] = value.y;
        node->resval.rpoint[Z] = value.z;
        return appendResbufNode(head, tail, node);
    }
};

template <typename T>
bool parseLispArg(const resbuf* node, T* out_value) {
    return LispTypeCodec<T>::decode(node, out_value);
}

bool hasNoMoreArgs(const resbuf* node) {
    return node == nullptr || (node->restype == RTNONE && node->rbnext == nullptr);
}

template <std::size_t Index, typename... Args>
typename std::enable_if<Index == sizeof...(Args), bool>::type parseTypedArgsRecursive(const resbuf* current, std::tuple<Args...>* /*out_values*/,
                                                                                       const resbuf** out_next) {
    if (out_next == nullptr) {
        return false;
    }
    *out_next = current;
    return true;
}

template <std::size_t Index, typename... Args>
typename std::enable_if<(Index < sizeof...(Args)), bool>::type parseTypedArgsRecursive(const resbuf* current, std::tuple<Args...>* out_values, const resbuf** out_next) {
    if (current == nullptr || out_values == nullptr || out_next == nullptr) {
        return false;
    }

    typedef typename std::tuple_element<Index, std::tuple<Args...>>::type ArgT;
    ArgT value{};
    if (!parseLispArg<ArgT>(current, &value)) {
        return false;
    }
    std::get<Index>(*out_values) = value;
    return parseTypedArgsRecursive<Index + 1, Args...>(current->rbnext, out_values, out_next);
}

template <typename... Args>
bool parseTypedArgs(const resbuf* args, std::tuple<Args...>* out_values) {
    if (out_values == nullptr) {
        return false;
    }
    const resbuf* tail = nullptr;
    if (!parseTypedArgsRecursive<0, Args...>(args, out_values, &tail)) {
        return false;
    }
    return hasNoMoreArgs(tail);
}

template <typename Fn, typename Tuple, std::size_t... Indices>
auto applyTupleImpl(Fn&& fn, Tuple& values, std::index_sequence<Indices...> indices) -> decltype(fn(std::get<Indices>(values)...)) {
    (void)indices;
    return std::forward<Fn>(fn)(std::get<Indices>(values)...);
}

template <typename Fn, typename Tuple>
auto applyTuple(Fn&& fn, Tuple& values) -> decltype(applyTupleImpl(std::forward<Fn>(fn), values, std::make_index_sequence<std::tuple_size<Tuple>::value>{})) {
    return applyTupleImpl(std::forward<Fn>(fn), values, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

template <typename... Args, typename Fn>
LispHandler makeTypedLispHandler(const ZTCHAR* function_name, Fn fn) {
    typedef typename std::decay<Fn>::type DecayedFn;
    return [name = std::wstring(function_name), callback = DecayedFn(fn)](resbuf* args) -> int {
        std::tuple<Args...> values;
        if (!parseTypedArgs<Args...>(args, &values)) {
            acutPrintf(_T("\n[LISP:%ls] 参数不匹配。"), name.c_str());
            zcedRetNil();
            return RTERROR;
        }
        return applyTuple(callback, values);
    };
}

std::vector<LispFunctionEntry>& lispFunctionTable() {
    static std::vector<LispFunctionEntry> table = {
        {_T("mfcEchoNoArgs"),
         makeTypedLispHandler<>(_T("mfcEchoNoArgs"), []() -> int {
             acutPrintf(_T("\n[LISP] mfcEchoNoArgs 被调用。"));
             zcedRetStr(_T("MFC Lisp NoArgs OK"));
             return RTNORM;
         })},
        {_T("mfcSumIntReal"),
         makeTypedLispHandler<int, double>(_T("mfcSumIntReal"), [](int left, double right) -> int {
             const double result = static_cast<double>(left) + right;
             acutPrintf(_T("\n[LISP] mfcSumIntReal 参数: %d, %.6f => %.6f"), left, right, result);
             zcedRetReal(result);
             return RTNORM;
         })},
        {_T("mfcDescribePoint"),
         makeTypedLispHandler<AcGePoint3d, std::wstring>(_T("mfcDescribePoint"), [](const AcGePoint3d& pt, const std::wstring& label) -> int {
             CString text;
             text.Format(_T("%ls:(%.3f, %.3f, %.3f)"), label.c_str(), pt.x, pt.y, pt.z);
             acutPrintf(_T("\n[LISP] mfcDescribePoint => %s"), text.GetString());
             zcedRetStr(text.GetString());
             return RTNORM;
         })},
    };
    return table;
}

bool& lispRegistrationState() {
    static bool registered = false;
    return registered;
}

bool appendResbufNode(resbuf** head, resbuf** tail, resbuf* node) {
    if (head == nullptr || tail == nullptr || node == nullptr) {
        return false;
    }
    node->rbnext = nullptr;
    if (*head == nullptr) {
        *head = node;
        *tail = node;
        return true;
    }
    (*tail)->rbnext = node;
    *tail = node;
    return true;
}

bool appendStringNode(resbuf** head, resbuf** tail, const std::wstring& text) {
    resbuf* node = acutNewRb(RTSTR);
    if (node == nullptr) {
        return false;
    }
    if (zcutNewString(text.c_str(), node->resval.rstring) != Zcad::eOk) {
        acutRelRb(node);
        return false;
    }
    return appendResbufNode(head, tail, node);
}

template <typename T>
bool appendInvokeArgNode(resbuf** head, resbuf** tail, const T& arg) {
    typedef typename std::decay<T>::type ArgT;
    return LispTypeCodec<ArgT>::encode(arg, head, tail);
}

bool appendInvokeTerminator(resbuf** head, resbuf** tail) {
    resbuf* node = acutNewRb(RTNONE);
    if (node == nullptr) {
        return false;
    }
    return appendResbufNode(head, tail, node);
}

template <typename... Args>
int invokeLispFunction(const std::wstring& function_name, const Args&... args) {
    resbuf* invoke_args = nullptr;
    resbuf* tail = nullptr;
    if (!appendStringNode(&invoke_args, &tail, function_name)) {
        acutPrintf(_T("\n[zcedInvoke] 构建函数名失败: %ls"), function_name.c_str());
        return RTERROR;
    }

    bool args_ok = true;
    std::initializer_list<int>{(args_ok = args_ok && appendInvokeArgNode(&invoke_args, &tail, args), 0)...};
    if (!args_ok) {
        acutPrintf(_T("\n[zcedInvoke] 构建参数失败: %ls"), function_name.c_str());
        acutRelRb(invoke_args);
        return RTERROR;
    }

    if (!appendInvokeTerminator(&invoke_args, &tail)) {
        acutPrintf(_T("\n[zcedInvoke] 构建RTNONE终止参数失败: %ls"), function_name.c_str());
        acutRelRb(invoke_args);
        return RTERROR;
    }

    resbuf* result = nullptr;
    const int invoke_status = zcedInvoke(invoke_args, &result);
    acutRelRb(invoke_args);

    if (invoke_status != RTNORM) {
        acutPrintf(_T("\n[zcedInvoke] 调用失败: %ls, status=%d"), function_name.c_str(), invoke_status);
        if (result != nullptr) {
            acutRelRb(result);
        }
        return invoke_status;
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


#endif
// 示例命令：显示MFC对话框
void MfcTestCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行MFC测试命令..."));

    // 获取CAD主窗口句柄
    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());

    // 创建并显示对话框
    CTestDialog dlg(pMainWnd);
    if (dlg.DoModal() == IDOK) {
        acutPrintf(_T("\n对话框已关闭。"));
    }
}

// 示例命令：显示ZWCAD ARX UI对话框
void MfcArxUiDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行ARX UI对话框命令..."));

    // 获取CAD主窗口句柄
    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());

    // 创建并显示对话框
    CArxUiDemoDialog dlg(pMainWnd);
    dlg.DoModal();
}

// 示例命令：创建简单实体
void MfcCreateEntity() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行创建实体命令..."));

    // 获取两个点
    AcGePoint3d pt1, pt2;
    if (acedGetPoint(nullptr, _T("\n选择第一个点: "), asDblArray(pt1)) != RTNORM) {
        acutPrintf(_T("\n命令已取消。"));
        return;
    }

    if (acedGetPoint(asDblArray(pt1), _T("\n选择第二个点: "), asDblArray(pt2)) != RTNORM) {
        acutPrintf(_T("\n命令已取消。"));
        return;
    }

    // 创建一条线
    AcDbLine* pLine = new AcDbLine(pt1, pt2);
    if (pLine == nullptr) {
        acutPrintf(_T("\n创建直线失败。"));
        return;
    }

    // 将线添加到数据库
    AcDbBlockTable* pBlockTable = nullptr;
    AcDbBlockTableRecord* pBlockTableRecord = nullptr;
    acdbHostApplicationServices()->workingDatabase()->getBlockTable(pBlockTable, AcDb::kForRead);
    pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord, AcDb::kForWrite);
    pBlockTable->close();

    AcDbObjectId lineId;
    pBlockTableRecord->appendAcDbEntity(lineId, pLine);
    pBlockTableRecord->close();
    pLine->close();

    acutPrintf(_T("\n直线创建成功！"));

    // 显示MFC消息框确认
    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    AfxMessageBox(_T("直线已成功创建！"), MB_OK | MB_ICONINFORMATION);
}

// 载入至文档命令
void MfcLoadToDocCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行载入至文档命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmLoadToDocDialog dlg(pMainWnd);
    dlg.DoModal();
}

// 构件浏览器命令
void MfcComponentBrowserCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmComponentBrowserDialog dlg(pMainWnd);
    dlg.DoModal();
}

// 示例命令：触发CAD另存为对话框
void MfcSaveAsDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行另存为测试命令（API模式）..."));

    AcApDocument* currentDoc = acDocManager->curDocument();
    if (currentDoc == nullptr || currentDoc->database() == nullptr) {
        acutPrintf(_T("\n无法获取当前文档。"));
        return;
    }

    CString defaultName = currentDoc->fileName();
    if (defaultName.IsEmpty()) {
        defaultName = _T("未命名.dwg");
    }

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    CAdUiFileDialog dlg(FALSE, _T("dwg"), defaultName, OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, _T("Drawing (*.dwg)|*.dwg||"), pMainWnd);
    if (dlg.DoModal() != IDOK) {
        acutPrintf(_T("\n已取消另存为。"));
        return;
    }

    CString targetPath = dlg.GetPathName();
    if (targetPath.IsEmpty()) {
        acutPrintf(_T("\n未选择有效保存路径。"));
        return;
    }

    CString lowerPath = targetPath;
    lowerPath.MakeLower();
    if (lowerPath.GetLength() < 4 || lowerPath.Right(4) != _T(".dwg")) {
        targetPath += _T(".dwg");
    }

    Acad::ErrorStatus status = currentDoc->database()->saveAs(targetPath.GetString(), true, AcDb::kDHL_CURRENT);
    if (status != Acad::eOk) {
        acutPrintf(_T("\n另存为失败，错误码: %d"), static_cast<int>(status));
        return;
    }

    acutPrintf(_T("\n另存为成功: %s"), targetPath.GetString());
}

void MfcTestRenameDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行重命名对话框测试命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmRenameTemplateDialog dlg(_T("示例构件A"), pMainWnd);
    const INT_PTR result = dlg.DoModal();

    if (result == IDOK) {
        acutPrintf(_T("\n[RenameDialog] 点击确定，新名称: %s"), dlg.getNewName().GetString());
        return;
    }

    if (dlg.isCancelLoad()) {
        acutPrintf(_T("\n[RenameDialog] 点击取消载入。"));
        return;
    }

    acutPrintf(_T("\n[RenameDialog] 对话框关闭，返回码: %Id"), result);
}

void MfcTestOverwriteDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行覆盖确认对话框测试命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmOverwriteConfirmDialog dlg(pMainWnd);
    const INT_PTR result = dlg.DoModal();
    const OverwriteChoice choice = dlg.choice();

    if (result == IDOK && choice == OverwriteChoice::kUpdateAll) {
        acutPrintf(_T("\n[OverwriteDialog] 选择: 全部更新。"));
        return;
    }

    if (result == IDOK && choice == OverwriteChoice::kSkipParams) {
        acutPrintf(_T("\n[OverwriteDialog] 选择: 不覆盖参数值。"));
        return;
    }

    acutPrintf(_T("\n[OverwriteDialog] 选择: 取消载入。"));
}

void MfcPropertyPermissionCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行属性权限设置命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmPropertyPermissionDialog dlg(pMainWnd);
    dlg.DoModal();
}

struct AppContextInvoker {
    template <typename Fn, typename... Args>
    explicit AppContextInvoker(Fn&& func, Args&&... args) {
        using Tuple = std::tuple<std::decay_t<Fn>, std::decay_t<Args>...>;
        auto Decay_copied = std::make_unique<Tuple>(std::forward<Fn>(func), std::forward<Args>(args)...);
        auto Invoker_proc = start<Tuple>(std::make_index_sequence<1 + sizeof...(Args)>{});
        acDocManager->executeInApplicationContext(Invoker_proc, Decay_copied.release());
    }

    template <typename Tuple, std::size_t... Indices>
    static constexpr auto start(std::index_sequence<Indices...> /*indices*/) noexcept {
        return &Invoke<Tuple, Indices...>;  // 返回函数指针
    }

    template <class Tuple, std::size_t... Indices>
    static void Invoke(void* RawVals) noexcept {
        const std::unique_ptr<Tuple> FnVals(static_cast<Tuple*>(RawVals));
        Tuple& Tup = *FnVals.get();
        std::invoke(std::move(std::get<Indices>(Tup))...);  // 真正的调用
    }
};

void MfcNewFamilyCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行新建族命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmNewFamilyDialog dlg(pMainWnd);
    if (dlg.DoModal() != IDOK) {
        acutPrintf(_T("\n已取消新建族。"));
        return;
    }

    const NewFamilyRequest request = dlg.request();
    AppContextInvoker call{[request]() {
        std::wstring error_message;
        if (!executeCreateNewFamily(request, &error_message)) {
            acutPrintf(_T("\n新建族失败。"));
            if (!error_message.empty()) {
                acutPrintf(_T("\n错误详情: %ls"), error_message.c_str());
            }
            return;
        }
        acutPrintf(_T("\n新建族流程执行成功。"));
        // 这里是CAD应用上下文中执行的代码，可以安全调用CAD API。
    }};
}

void MfcNewFamilyTempDemoCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行新建族临时文档链路 Demo..."));

    CWnd* main_wnd = CWnd::FromHandle(adsw_acadMainWnd());
    CAdUiFileDialog dlg(TRUE, _T("ztf"), _T(""), OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
                        _T("模板文件 (*.ztf)|*.ztf|Drawing 文件 (*.dwg)|*.dwg|所有文件 (*.*)|*.*||"), main_wnd);
    if (dlg.DoModal() != IDOK) {
        acutPrintf(_T("\n已取消 Demo。"));
        return;
    }

    const std::wstring template_path = dlg.GetPathName().GetString();
    AppContextInvoker call{[template_path]() {
        std::wstring error_message;
        if (!executeCreateNewFamilyTempDemo(template_path, &error_message)) {
            acutPrintf(_T("\n[Demo] 临时文档链路失败。"));
            if (!error_message.empty()) {
                acutPrintf(_T("\n[Demo] 错误详情: %ls"), error_message.c_str());
            }
            return;
        }
        acutPrintf(_T("\n[Demo] 临时文档链路执行成功。"));
    }};
}

#if 0
int MfcRegisterLispCommands() {
    if (lispRegistrationState()) {
        return RTNORM;
    }
    const auto& table = lispFunctionTable();
    for (size_t i = 0; i < table.size(); ++i) {
        if (zcedDefun(table[i].function_name, static_cast<int>(i)) == 0) {
            acutPrintf(_T("\n[LISP] 注册失败: %s"), table[i].function_name);
            return RTERROR;
        }
    }
    acutPrintf(_T("\n[LISP] 已注册 %d 个函数。"), static_cast<int>(table.size()));
    lispRegistrationState() = true;
    return RTNORM;
}

int MfcUnregisterLispCommands() {
    if (!lispRegistrationState()) {
        return RTNORM;
    }
    const auto& table = lispFunctionTable();
    for (size_t i = 0; i < table.size(); ++i) {
        if (zcedUndef(table[i].function_name, static_cast<int>(i)) == 0) {
            acutPrintf(_T("\n[LISP] 反注册失败: %s"), table[i].function_name);
            return RTERROR;
        }
    }
    lispRegistrationState() = false;
    return RTNORM;
}

int MfcDispatchLispCommand() {
    const auto& table = lispFunctionTable();
    const int function_code = zcedGetFunCode();
    if (function_code < 0 || function_code >= static_cast<int>(table.size())) {
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
#endif
