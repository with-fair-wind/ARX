#include <Utils/string_utils.h>

#include <cwctype>

namespace {
const std::wstring& whiteSpaceChars() {
    static const std::wstring chars = L" \t\r\n";
    return chars;
}
}  // namespace

std::wstring StringUtils::trimCopy(const std::wstring& text) {
    const std::size_t begin = text.find_first_not_of(whiteSpaceChars());
    if (begin == std::wstring::npos) {
        return L"";
    }
    const std::size_t end = text.find_last_not_of(whiteSpaceChars());
    return text.substr(begin, end - begin + 1);
}

std::wstring StringUtils::trimLeftCopy(const std::wstring& text) {
    const std::size_t begin = text.find_first_not_of(whiteSpaceChars());
    if (begin == std::wstring::npos) {
        return L"";
    }
    return text.substr(begin);
}

std::wstring StringUtils::trimRightCopy(const std::wstring& text) {
    const std::size_t end = text.find_last_not_of(whiteSpaceChars());
    if (end == std::wstring::npos) {
        return L"";
    }
    return text.substr(0, end + 1);
}

void StringUtils::trimInPlace(std::wstring& text) {
    text = trimCopy(text);
}

std::wstring StringUtils::toLowerCopy(std::wstring text) {
    toLowerInPlace(text);
    return text;
}

std::wstring StringUtils::toUpperCopy(std::wstring text) {
    toUpperInPlace(text);
    return text;
}

void StringUtils::toLowerInPlace(std::wstring& text) {
    for (wchar_t& ch : text) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
}

void StringUtils::toUpperInPlace(std::wstring& text) {
    for (wchar_t& ch : text) {
        ch = static_cast<wchar_t>(std::towupper(ch));
    }
}

bool StringUtils::isWhitespaceOnly(const std::wstring& text) {
    return text.find_first_not_of(whiteSpaceChars()) == std::wstring::npos;
}

bool StringUtils::equals(const std::wstring& left, const std::wstring& right) {
    return left == right;
}

bool StringUtils::equalsIgnoreCase(const std::wstring& left, const std::wstring& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::towlower(left[i]) != std::towlower(right[i])) {
            return false;
        }
    }
    return true;
}

bool StringUtils::startsWith(const std::wstring& text, const std::wstring& prefix) {
    if (prefix.size() > text.size()) {
        return false;
    }
    return text.compare(0, prefix.size(), prefix) == 0;
}

bool StringUtils::startsWithIgnoreCase(const std::wstring& text, const std::wstring& prefix) {
    if (prefix.size() > text.size()) {
        return false;
    }
    return equalsIgnoreCase(text.substr(0, prefix.size()), prefix);
}

bool StringUtils::endsWith(const std::wstring& text, const std::wstring& suffix) {
    if (suffix.size() > text.size()) {
        return false;
    }
    const std::size_t offset = text.size() - suffix.size();
    return text.compare(offset, suffix.size(), suffix) == 0;
}

bool StringUtils::endsWithIgnoreCase(const std::wstring& text, const std::wstring& suffix) {
    if (suffix.size() > text.size()) {
        return false;
    }
    const std::size_t offset = text.size() - suffix.size();
    return equalsIgnoreCase(text.substr(offset), suffix);
}

bool StringUtils::contains(const std::wstring& text, const std::wstring& value) {
    if (value.empty()) {
        return true;
    }
    return text.find(value) != std::wstring::npos;
}

bool StringUtils::containsIgnoreCase(const std::wstring& text, const std::wstring& value) {
    if (value.empty()) {
        return true;
    }
    return toLowerCopy(text).find(toLowerCopy(value)) != std::wstring::npos;
}

std::vector<std::wstring> StringUtils::split(const std::wstring& text, wchar_t delimiter, bool skipEmpty) {
    std::vector<std::wstring> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t pos = text.find(delimiter, start);
        const std::size_t end = (pos == std::wstring::npos) ? text.size() : pos;
        std::wstring token = text.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            parts.push_back(std::move(token));
        }
        if (pos == std::wstring::npos) {
            break;
        }
        start = pos + 1;
    }
    return parts;
}

std::wstring StringUtils::join(const std::vector<std::wstring>& parts, const std::wstring& delimiter) {
    if (parts.empty()) {
        return L"";
    }
    std::wstring result;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            result += delimiter;
        }
        result += parts[i];
    }
    return result;
}

std::wstring StringUtils::replaceAllCopy(std::wstring text, const std::wstring& from, const std::wstring& to) {
    replaceAllInPlace(text, from, to);
    return text;
}

void StringUtils::replaceAllInPlace(std::wstring& text, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) {
        return;
    }
    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::wstring::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}
