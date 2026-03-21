#pragma once

#include <string>
#include <vector>

// C++14 字符串通用工具（宽字符）。
class StringUtils final {
   public:
    StringUtils() = delete;

    // 去除首尾空白。
    static std::wstring trimCopy(const std::wstring& text);
    static std::wstring trimLeftCopy(const std::wstring& text);
    static std::wstring trimRightCopy(const std::wstring& text);
    static void trimInPlace(std::wstring& text);

    // 大小写转换。
    static std::wstring toLowerCopy(std::wstring text);
    static std::wstring toUpperCopy(std::wstring text);
    static void toLowerInPlace(std::wstring& text);
    static void toUpperInPlace(std::wstring& text);

    // 判空/比较。
    static bool isWhitespaceOnly(const std::wstring& text);
    static bool equals(const std::wstring& left, const std::wstring& right);
    static bool equalsIgnoreCase(const std::wstring& left, const std::wstring& right);

    // 前后缀/包含判断。
    static bool startsWith(const std::wstring& text, const std::wstring& prefix);
    static bool startsWithIgnoreCase(const std::wstring& text, const std::wstring& prefix);
    static bool endsWith(const std::wstring& text, const std::wstring& suffix);
    static bool endsWithIgnoreCase(const std::wstring& text, const std::wstring& suffix);
    static bool contains(const std::wstring& text, const std::wstring& value);
    static bool containsIgnoreCase(const std::wstring& text, const std::wstring& value);

    // 拆分与拼接。
    static std::vector<std::wstring> split(const std::wstring& text, wchar_t delimiter, bool skipEmpty = false);
    static std::wstring join(const std::vector<std::wstring>& parts, const std::wstring& delimiter);

    // 替换。
    static std::wstring replaceAllCopy(std::wstring text, const std::wstring& from, const std::wstring& to);
    static void replaceAllInPlace(std::wstring& text, const std::wstring& from, const std::wstring& to);
};
