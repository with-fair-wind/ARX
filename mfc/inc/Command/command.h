#pragma once

// 示例命令：显示MFC对话框
void MfcTestCommand();

// 示例命令：显示ARX UI对话框
void MfcArxUiDialogCommand();

// 示例命令：创建简单实体
void MfcCreateEntity();

// 载入至文档命令
void MfcLoadToDocCommand();

// 构件浏览器命令
void MfcComponentBrowserCommand();

// 族模板管理器命令（ID 驱动版本）
void MfcTemplateManagerCommand();

// 另存为测试命令
void MfcSaveAsDialogCommand();

// 重命名模板对话框测试命令
void MfcTestRenameDialogCommand();

// 覆盖确认对话框测试命令
void MfcTestOverwriteDialogCommand();

// 属性权限设置对话框命令
void MfcPropertyPermissionCommand();

// 新建族对话框命令
void MfcNewFamilyCommand();

// 视图关联工作平面对话框命令（非模态）
void MfcViewWorkPlaneCommand();

// 临时文档链路测试命令
void MfcNewFamilyTempDemoCommand();

// zcedInvoke 测试命令（不同传参）
void MfcLispInvokeNoArgsCommand();
void MfcLispInvokeIntRealCommand();
void MfcLispInvokePointStringCommand();
void MfcLispValueCategoryTestCommand();
void MfcLispDynamicApiTestCommand();