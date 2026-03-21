#pragma once

#include <string>

// C++14 路径通用工具（避免依赖 C++17 std::filesystem）。
class PathUtils final {
   public:
    PathUtils() = delete;

    // 路径是否存在（文件或目录）。
    static bool exists(const std::wstring& path);

    // 判断是否是存在的文件（非目录）。
    static bool isFile(const std::wstring& path);

    // 判断是否是存在的目录。
    static bool isDirectory(const std::wstring& path);

    // 路径拼接（兼容末尾分隔符）。
    static std::wstring join(const std::wstring& left, const std::wstring& right);

    // 绝对路径判断（盘符路径 + UNC 路径）。
    static bool isAbsolute(const std::wstring& path);

    // 获取文件名（不含目录）。
    static std::wstring filename(const std::wstring& path);

    // 获取父目录（不含末尾分隔符）。
    static std::wstring parentPath(const std::wstring& path);

    // 获取扩展名（含 '.'；无扩展名返回空）。
    static std::wstring extension(const std::wstring& path);

    // 获取不含扩展名的文件名。
    static std::wstring stem(const std::wstring& path);

    // 统一路径分隔符。
    static std::wstring normalizeSeparators(const std::wstring& path, wchar_t separator = L'\\');

    // 仅用于比较：trim + 统一分隔符 + 小写化。
    static std::wstring normalizeForCompare(const std::wstring& path);

    // 忽略大小写 + 分隔符差异比较。
    static bool equivalentForCompare(const std::wstring& left, const std::wstring& right);
};
