#pragma once
#include <algorithm>
#include <memory>
#include <vector>

// ============================================
// 通用规约模式 - 支持任意类型
// ============================================

/// @brief 规约接口 - 用于定义过滤条件
/// @tparam T 被过滤的对象类型
template <typename T>
struct Specification {
    virtual ~Specification() = default;
    [[nodiscard]] virtual bool is_satisfied(const T& item) const = 0;

    // 便捷的组合操作符
    [[nodiscard]] auto operator&&(const Specification<T>& other) const -> std::unique_ptr<Specification<T>>;
    [[nodiscard]] auto operator||(const Specification<T>& other) const -> std::unique_ptr<Specification<T>>;
    [[nodiscard]] auto operator!() const -> std::unique_ptr<Specification<T>>;
};

// ============================================
// 组合规约 - 支持多个条件
// ============================================

/// @brief AND 规约 - 支持多个条件，所有条件都必须满足
/// @tparam T 被过滤的对象类型
template <typename T>
struct AndSpecification : Specification<T> {
    // 支持两个条件的构造
    AndSpecification(const Specification<T>& first, const Specification<T>& second) {
        specs.push_back(&first);
        specs.push_back(&second);
    }

    // 支持多个条件的构造（可变参数）
    template <typename... Specs>
    explicit AndSpecification(const Specs&... specs_) {
        (specs.push_back(&specs_), ...);
    }

    // 支持容器构造
    explicit AndSpecification(const std::vector<const Specification<T>*>& specs_) : specs(specs_) {}

    [[nodiscard]] bool is_satisfied(const T& item) const override {
        return std::all_of(specs.begin(), specs.end(), [&item](const Specification<T>* spec) { return spec->is_satisfied(item); });
    }

   private:
    std::vector<const Specification<T>*> specs;
};

/// @brief OR 规约 - 支持多个条件，任一条件满足即可
/// @tparam T 被过滤的对象类型
template <typename T>
struct OrSpecification : Specification<T> {
    // 支持两个条件的构造
    OrSpecification(const Specification<T>& first, const Specification<T>& second) {
        specs.push_back(&first);
        specs.push_back(&second);
    }

    // 支持多个条件的构造（可变参数）
    template <typename... Specs>
    explicit OrSpecification(const Specs&... specs_) {
        (specs.push_back(&specs_), ...);
    }

    // 支持容器构造
    explicit OrSpecification(const std::vector<const Specification<T>*>& specs_) : specs(specs_) {}

    [[nodiscard]] bool is_satisfied(const T& item) const override {
        return std::any_of(specs.begin(), specs.end(), [&item](const Specification<T>* spec) { return spec->is_satisfied(item); });
    }

   private:
    std::vector<const Specification<T>*> specs;
};

/// @brief NOT 规约 - 条件取反
/// @tparam T 被过滤的对象类型
template <typename T>
struct NotSpecification : Specification<T> {
    explicit NotSpecification(const Specification<T>& spec) : inner(spec) {}
    [[nodiscard]] bool is_satisfied(const T& item) const override { return !inner.is_satisfied(item); }

   private:
    const Specification<T>& inner;
};

// ============================================
// 便捷的组合操作符实现
// ============================================

template <typename T>
auto Specification<T>::operator&&(const Specification<T>& other) const -> std::unique_ptr<Specification<T>> {
    return std::make_unique<AndSpecification<T>>(*this, other);
}

template <typename T>
auto Specification<T>::operator||(const Specification<T>& other) const -> std::unique_ptr<Specification<T>> {
    return std::make_unique<OrSpecification<T>>(*this, other);
}

template <typename T>
auto Specification<T>::operator!() const -> std::unique_ptr<Specification<T>> {
    return std::make_unique<NotSpecification<T>>(*this);
}

// ============================================
// 通用过滤器 - 支持任意类型
// ============================================

/// @brief 过滤器接口 - 用于执行过滤操作
/// @tparam T 被过滤的对象类型
template <typename T>
struct Filter {
    virtual ~Filter() = default;

    /// @brief 过滤 std::vector
    [[nodiscard]] virtual std::vector<T> filter(const std::vector<T>& items, const Specification<T>& spec) const = 0;

    /// @brief 过滤任意容器（通过迭代器）
    template <typename Container>
    [[nodiscard]] std::vector<T> filter(const Container& items, const Specification<T>& spec) const {
        return filter(std::begin(items), std::end(items), spec);
    }

    /// @brief 过滤迭代器范围
    template <typename It>
    [[nodiscard]] std::vector<T> filter(It first, It last, const Specification<T>& spec) const {
        std::vector<T> res;
        using cat = typename std::iterator_traits<It>::iterator_category;
        if constexpr (std::is_base_of_v<std::random_access_iterator_tag, cat>) {
            res.reserve(static_cast<size_t>(last - first));
        }
        for (; first != last; ++first) {
            if (spec.is_satisfied(*first)) {
                res.push_back(*first);
            }
        }
        return res;
    }
};

/// @brief 基础过滤器实现
/// @tparam T 被过滤的对象类型
template <typename T>
struct BasicFilter : Filter<T> {
    [[nodiscard]] std::vector<T> filter(const std::vector<T>& items, const Specification<T>& spec) const override {
        std::vector<T> res;
        res.reserve(items.size());
        for (const auto& item : items) {
            if (spec.is_satisfied(item)) {
                res.push_back(item);
            }
        }
        return res;
    }
};

// ============================================
// 辅助函数 - 更方便地创建组合规约
// ============================================

/// @brief 创建 AND 规约（可变参数）
template <typename T, typename... Specs>
auto make_and(const Specs&... specs) {
    return AndSpecification<T>(specs...);
}

/// @brief 创建 OR 规约（可变参数）
template <typename T, typename... Specs>
auto make_or(const Specs&... specs) {
    return OrSpecification<T>(specs...);
}

/// @brief 创建 NOT 规约
template <typename T>
auto make_not(const Specification<T>& spec) {
    return NotSpecification<T>(spec);
}