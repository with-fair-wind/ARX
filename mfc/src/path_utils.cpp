#include <Utils/path_utils.h>
#include <Utils/string_utils.h>

#include <windows.h>

bool PathUtils::exists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    return ::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool PathUtils::isFile(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    const DWORD attrs = ::GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool PathUtils::isDirectory(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    const DWORD attrs = ::GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::wstring PathUtils::join(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    if (left.back() == L'\\' || left.back() == L'/') {
        return left + right;
    }
    return left + L"\\" + right;
}

bool PathUtils::isAbsolute(const std::wstring& path) {
    if (path.size() >= 2 && path[1] == L':') {
        return true;
    }
    return path.size() >= 2 && ((path[0] == L'\\' && path[1] == L'\\') || (path[0] == L'/' && path[1] == L'/'));
}

std::wstring PathUtils::filename(const std::wstring& path) {
    if (path.empty()) {
        return L"";
    }
    const std::size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::wstring PathUtils::parentPath(const std::wstring& path) {
    if (path.empty()) {
        return L"";
    }
    const std::size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, pos);
}

std::wstring PathUtils::extension(const std::wstring& path) {
    const std::wstring fileName = filename(path);
    const std::size_t pos = fileName.find_last_of(L'.');
    if (pos == std::wstring::npos || pos == 0) {
        return L"";
    }
    return fileName.substr(pos);
}

std::wstring PathUtils::stem(const std::wstring& path) {
    const std::wstring fileName = filename(path);
    const std::size_t pos = fileName.find_last_of(L'.');
    if (pos == std::wstring::npos || pos == 0) {
        return fileName;
    }
    return fileName.substr(0, pos);
}

std::wstring PathUtils::normalizeSeparators(const std::wstring& path, wchar_t separator) {
    std::wstring normalized = path;
    for (wchar_t& ch : normalized) {
        if (ch == L'\\' || ch == L'/') {
            ch = separator;
        }
    }
    return normalized;
}

std::wstring PathUtils::normalizeForCompare(const std::wstring& path) {
    return StringUtils::toLowerCopy(normalizeSeparators(StringUtils::trimCopy(path), L'\\'));
}

bool PathUtils::equivalentForCompare(const std::wstring& left, const std::wstring& right) {
    return normalizeForCompare(left) == normalizeForCompare(right);
}
