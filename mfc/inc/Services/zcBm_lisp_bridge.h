#pragma once

#include <Utils/zcBm_resbuf_codec.h>
#include <acedads.h>
#include <acutads.h>
#include <zAcString.h>

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

namespace ZcBmLispBridge {
inline constexpr int kZcBmLispFuncCodeBase = 20000;
inline constexpr int kZcBmLispDispatchNotHandled = -1;

using ZcBmLispHandler = std::function<int(resbuf*)>;

namespace detail {
template <typename T>
bool parseLispArg(const resbuf* node, T* out_value) {
    if (node == nullptr || out_value == nullptr) {
        return false;
    }
    return ZcBmResbufCodec<T>::decode(node, static_cast<short>(node->restype), out_value);
}

template <typename... Args>
using DecayedArgsTuple = std::tuple<typename std::decay<Args>::type...>;

inline bool hasNoMoreArgs(const resbuf* node) { return node == nullptr || (node->restype == RTNONE && node->rbnext == nullptr); }

template <std::size_t Index, typename Tuple>
bool parseTupleElement(const resbuf*& current, Tuple* out_values) {
    if (current == nullptr || out_values == nullptr) {
        return false;
    }
    using ArgT = typename std::tuple_element<Index, Tuple>::type;
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
}  // namespace detail

template <typename... Args, typename Fn>
ZcBmLispHandler makeTypedLispHandler(const ZcString& function_name, Fn&& fn) {
    using DecayedFn = typename std::decay<Fn>::type;
    return [name = function_name, fn_capture = DecayedFn(std::forward<Fn>(fn))](resbuf* args) -> int {
        detail::DecayedArgsTuple<Args...> values;
        if (!detail::parseTypedArgs<Args...>(args, &values)) {
            acutPrintf(_T("\n[LISP:%s] 参数不匹配。"), name.kTCharPtr());
            zcedRetNil();
            return RTERROR;
        }
        return detail::invokeFromTuple(fn_capture, values);
    };
}

struct ZcBmLispFunctionEntry {
    ZcString function_name;
    ZcBmLispHandler handler;

    ZcBmLispFunctionEntry() = default;
    ZcBmLispFunctionEntry(const ZcString& func_name, ZcBmLispHandler func_handler)
        : function_name(func_name), handler(std::move(func_handler)) {}
    template <typename... Args, typename Fn, typename DecayedFn = typename std::decay<Fn>::type,
              typename std::enable_if<!std::is_convertible<DecayedFn, ZcBmLispHandler>::value, int>::type = 0>
    explicit ZcBmLispFunctionEntry(const ZcString& func_name, Fn&& fn)
        : function_name(func_name), handler(makeTypedLispHandler<Args...>(function_name, std::forward<Fn>(fn))) {}
};

struct ZcBmLispCache {
    ZcBmLispCache() = default;
    explicit ZcBmLispCache(const std::vector<ZcBmLispFunctionEntry>& table) : m_table(table) {
        for (size_t i = 0; i < m_table.size(); ++i) {
            if (m_table[i].handler != nullptr && !m_table[i].function_name.isEmpty()) {
                m_name_to_index[makeKey(m_table[i].function_name)] = i;
            } else {
                m_free_indices.push_back(i);
            }
        }
    }

    template <typename... Args, typename Fn, typename DecayedFn = typename std::decay<Fn>::type,
              typename std::enable_if<!std::is_convertible<DecayedFn, ZcBmLispHandler>::value, int>::type = 0>
    int push(const ZcString& function_name, Fn&& fn, size_t* out_index = nullptr, ZcBmLispHandler* out_previous_handler = nullptr) {
        return push(function_name, makeTypedLispHandler<Args...>(function_name, std::forward<Fn>(fn)), out_index, out_previous_handler);
    }

    int push(const ZcString& function_name, const ZcBmLispHandler& handler, size_t* out_index = nullptr, ZcBmLispHandler* out_previous_handler = nullptr) {
        if (function_name.isEmpty() || handler == nullptr) {
            return RTERROR;
        }

        const auto key = makeKey(function_name);
        const auto index_it = m_name_to_index.find(key);
        if (index_it != m_name_to_index.end()) {
            const size_t index = index_it->second;
            if (index < m_table.size() && m_table[index].handler != nullptr) {
                if (out_previous_handler != nullptr) {
                    *out_previous_handler = m_table[index].handler;
                }
                m_table[index].handler = handler;
                if (out_index != nullptr) {
                    *out_index = index;
                }
                return RTNORM;
            }
            m_name_to_index.erase(index_it);
        }

        if (out_previous_handler != nullptr) {
            *out_previous_handler = ZcBmLispHandler{};
        }

        size_t index = m_table.size();
        while (!m_free_indices.empty()) {
            const size_t candidate = m_free_indices.back();
            m_free_indices.pop_back();
            if (candidate < m_table.size() && m_table[candidate].handler == nullptr) {
                index = candidate;
                break;
            }
        }

        if (index == m_table.size()) {
            m_table.emplace_back(function_name, handler);
            index = m_table.size() - 1;
        } else {
            m_table[index].function_name = function_name;
            m_table[index].handler = handler;
        }
        m_name_to_index[key] = index;
        if (out_index != nullptr) {
            *out_index = index;
        }
        return RTNORM;
    }

    int remove(const ZcString& function_name,
               size_t* out_index = nullptr,
               ZcString* out_removed_name = nullptr,
               ZcBmLispHandler* out_removed_handler = nullptr) {
        if (function_name.isEmpty()) {
            return RTERROR;
        }
        const auto key = makeKey(function_name);
        const auto index_it = m_name_to_index.find(key);
        if (index_it == m_name_to_index.end()) {
            return RTERROR;
        }

        const size_t index = index_it->second;
        if (index >= m_table.size() || m_table[index].handler == nullptr) {
            m_name_to_index.erase(index_it);
            return RTERROR;
        }

        if (out_index != nullptr) {
            *out_index = index;
        }
        if (out_removed_name != nullptr) {
            *out_removed_name = m_table[index].function_name;
        }
        if (out_removed_handler != nullptr) {
            *out_removed_handler = m_table[index].handler;
        }

        m_table[index].handler = nullptr;
        m_table[index].function_name = ZcString();
        m_name_to_index.erase(index_it);
        if (std::find(m_free_indices.begin(), m_free_indices.end(), index) == m_free_indices.end()) {
            m_free_indices.push_back(index);
        }
        return RTNORM;
    }

   private:
    static std::wstring makeKey(const ZcString& function_name) {
        return function_name.isEmpty() ? std::wstring{} : std::wstring(function_name.kTCharPtr());
    }

    friend int ZcBmRegisterLispCommands(ZcBmLispCache& cache);
    friend int ZcBmUnregisterLispCommands(ZcBmLispCache& cache);
    friend int ZcBmDispatchLispCommand(ZcBmLispCache& cache);

    std::vector<ZcBmLispFunctionEntry> m_table;
    std::unordered_map<std::wstring, size_t> m_name_to_index;
    std::vector<size_t> m_free_indices;
};

template <typename T>
struct ZcBmLispInvokeRestype;

template <>
struct ZcBmLispInvokeRestype<int> {
    static constexpr int value = RTLONG;
};
template <>
struct ZcBmLispInvokeRestype<std::int16_t> {
    static constexpr int value = RTSHORT;
};
template <>
struct ZcBmLispInvokeRestype<std::int64_t> {
    static constexpr int value = RTINT64;
};
template <>
struct ZcBmLispInvokeRestype<bool> {
    static constexpr int value = RTSHORT;
};
template <>
struct ZcBmLispInvokeRestype<double> {
    static constexpr int value = RTREAL;
};
template <>
struct ZcBmLispInvokeRestype<std::wstring> {
    static constexpr int value = RTSTR;
};
template <>
struct ZcBmLispInvokeRestype<ZcString> {
    static constexpr int value = RTSTR;
};
template <>
struct ZcBmLispInvokeRestype<ZTCHAR*> {
    static constexpr int value = RTSTR;
};
template <>
struct ZcBmLispInvokeRestype<const ZTCHAR*> {
    static constexpr int value = RTSTR;
};
template <>
struct ZcBmLispInvokeRestype<AcGePoint3d> {
    static constexpr int value = RT3DPOINT;
};
template <>
struct ZcBmLispInvokeRestype<zds_name> {
    static constexpr int value = RTENAME;
};
template <>
struct ZcBmLispInvokeRestype<AcDbObjectId> : ZcBmLispInvokeRestype<zds_name> {};

int ZcBmRegisterLispCommands(ZcBmLispCache& cache);
int ZcBmUnregisterLispCommands(ZcBmLispCache& cache);
int ZcBmDispatchLispCommand(ZcBmLispCache& cache);

int ZcBmAddLispFunction(ZcBmLispCache& cache, const ZcString& function_name, const ZcBmLispHandler& handler, int* out_func_code = nullptr);

template <typename... Args, typename Fn, typename DecayedFn = typename std::decay<Fn>::type,
          typename std::enable_if<!std::is_convertible<DecayedFn, ZcBmLispHandler>::value, int>::type = 0>
int ZcBmAddLispFunction(ZcBmLispCache& cache, const ZcString& function_name, Fn&& fn, int* out_func_code = nullptr) {
    return ZcBmAddLispFunction(cache, function_name, makeTypedLispHandler<Args...>(function_name, std::forward<Fn>(fn)), out_func_code);
}

int invokeLispFunction(resbuf* args, resbuf** out_result = nullptr);

template <typename... Args>
int invokeLispFunction(const ZcString& function_name, resbuf** out_result, Args&&... args) {
    ZcBmResbufChain chain;
    if (!ZcBmResbufCodec<ZcString>::encode(RTSTR, function_name, &chain)) {
        return RTERROR;
    }

    bool args_ok = true;
    std::initializer_list<int>{
        (args_ok = args_ok && ZcBmResbufCodec<typename std::decay<Args>::type>::encode(
                                  ZcBmLispInvokeRestype<typename std::decay<Args>::type>::value,
                                  std::forward<Args>(args),
                                  &chain),
         0)...};
    if (!args_ok || !chain.appendTerminator()) {
        chain.release();
        return RTERROR;
    }

    return invokeLispFunction(chain.detach(), out_result);
}

template <typename... Args>
int invokeLispFunction(const ZcString& function_name, Args&&... args) {
    return invokeLispFunction(function_name, static_cast<resbuf**>(nullptr), std::forward<Args>(args)...);
}
int ZcBmRemoveLispFunction(ZcBmLispCache& cache, const ZcString& function_name);
}  // namespace ZcBmLispBridge

// 向后兼容接口（默认缓存）
using ZcBmLispRawHandler = int (*)(resbuf*);
static constexpr int kZcBmLispDispatchNotHandled = ZcBmLispBridge::kZcBmLispDispatchNotHandled;

int ZcBmRegisterLispCommands();
int ZcBmUnregisterLispCommands();
int ZcBmDispatchLispCommand();
int ZcBmAddLispFunction(const ZTCHAR* function_name, const ZcBmLispRawHandler& handler, int* out_func_code = nullptr);
int ZcBmRemoveLispFunction(const ZTCHAR* function_name);
