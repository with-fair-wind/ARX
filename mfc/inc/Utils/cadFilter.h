#pragma once

#include <BcSketchFuncXData.h>
#include <Func/filter.h>

#include <algorithm>
#include <cmath>
#include <variant>

#include "dbidar.h"

// ============================================
// CAD 实体过滤规约
// ============================================

/// @brief XData 值类型（支持字符串、整数、浮点数）
using XDataValue = std::variant<CString, int, double>;

/// @brief XData 匹配条件
struct XDataCondition {
    CString appName;   ///< XData 应用名称   // NOLINT(misc-non-private-member-variables-in-classes)
    XDataValue value;  ///< 期望的值         // NOLINT(misc-non-private-member-variables-in-classes)
    double tolerance;  ///< 浮点数比较容差（仅对 double 有效）  // NOLINT(misc-non-private-member-variables-in-classes)

    XDataCondition(const CString& app, const CString& val) : appName(app), value(val), tolerance(1e-6) {}  // NOLINT(bugprone-easily-swappable-parameters)
    XDataCondition(const CString& app, int val) : appName(app), value(val), tolerance(1e-6) {}
    XDataCondition(const CString& app, double val, double tol) : appName(app), value(val), tolerance(tol) {}  // NOLINT(bugprone-easily-swappable-parameters)
};

/// @brief 通用 XData 过滤规约，支持多 appname 和多类型值匹配
/// @details 可以匹配单个或多个 appname-value 对，支持字符串、整数、浮点数，支持 AND/OR 逻辑
struct XDataSpecification : Specification<AcDbObjectId> {
    enum class MatchMode : std::uint8_t {
        Any,  ///< 匹配任意一个条件即可（OR 逻辑）
        All   ///< 必须匹配所有条件（AND 逻辑）
    };

    /// @brief 构造函数：单个字符串值匹配
    /// @param appName XData 应用名称
    /// @param xdataName 要匹配的 XData 值
    XDataSpecification(const CString& appName, const CString& xdataName) : m_matchMode(MatchMode::Any) { m_conditions.emplace_back(appName, xdataName); }

    /// @brief 构造函数：单个整数值匹配
    /// @param appName XData 应用名称
    /// @param xdataValue 要匹配的整数值
    XDataSpecification(const CString& appName, int xdataValue) : m_matchMode(MatchMode::Any) { m_conditions.emplace_back(appName, xdataValue); }

    /// @brief 构造函数：单个浮点数值匹配
    /// @param appName XData 应用名称
    /// @param xdataValue 要匹配的浮点数值
    /// @param tolerance 浮点数比较容差
    XDataSpecification(const CString& appName, double xdataValue, double tolerance = 1e-6) : m_matchMode(MatchMode::Any) { m_conditions.emplace_back(appName, xdataValue, tolerance); }

    /// @brief 构造函数：多个条件匹配
    explicit XDataSpecification(std::vector<XDataCondition> conditions, MatchMode mode = MatchMode::Any) : m_conditions(std::move(conditions)), m_matchMode(mode) {}

    [[nodiscard]] bool is_satisfied(const AcDbObjectId& item) const override {
        if (m_matchMode == MatchMode::Any) {
            // OR 逻辑：任意一个条件满足即可
            return std::any_of(m_conditions.begin(), m_conditions.end(), [&](const XDataCondition& cond) { return matchCondition(item, cond); });
        }

        // AND 逻辑：所有条件都必须满足
        return !m_conditions.empty() && std::all_of(m_conditions.begin(), m_conditions.end(), [&](const XDataCondition& cond) { return matchCondition(item, cond); });
    }

    /// @brief 流式 API：添加字符串条件
    XDataSpecification& withString(const CString& appName, const CString& value) {
        m_conditions.emplace_back(appName, value);
        return *this;
    }

    /// @brief 流式 API：添加整数条件
    XDataSpecification& withInt(const CString& appName, int value) {
        m_conditions.emplace_back(appName, value);
        return *this;
    }

    /// @brief 流式 API：添加浮点数条件
    XDataSpecification& withDouble(const CString& appName, double value, double tolerance = 1e-6) {
        m_conditions.emplace_back(appName, value, tolerance);
        return *this;
    }

    /// @brief 设置匹配模式
    XDataSpecification& setMatchMode(MatchMode mode) {
        m_matchMode = mode;
        return *this;
    }

   private:
    std::vector<XDataCondition> m_conditions;  ///< 条件列表
    MatchMode m_matchMode;                     ///< 匹配模式

    /// @brief 匹配单个条件
    [[nodiscard]] static bool matchCondition(const AcDbObjectId& item, const XDataCondition& condition) {
        AcDbObject* pObj = nullptr;
        if (acdbOpenObject(pObj, item, AcDb::kForRead) != Acad::eOk) {
            return false;
        }

        resbuf* pXData = pObj->xData(condition.appName);
        pObj->close();

        if (pXData == nullptr) {
            return false;
        }

        bool matched = std::visit([&](const auto& expectedValue) -> bool { return matchXDataValue(pXData, expectedValue, condition.tolerance); }, condition.value);

        acutRelRb(pXData);
        return matched;
    }

    /// @brief 匹配 XData 值（字符串）
    [[nodiscard]] static bool matchXDataValue(const resbuf* pXData, const CString& expectedValue, double /*tolerance*/) {
        for (const resbuf* rb = pXData; rb != nullptr; rb = rb->rbnext) {
            if (rb->restype == kDxfXdAsciiString) {
                if (CString(rb->resval.rstring).Compare(expectedValue) == 0) {
                    return true;
                }
            }
        }
        return false;
    }

    /// @brief 匹配 XData 值（整数）
    [[nodiscard]] static bool matchXDataValue(const resbuf* pXData, int expectedValue, double /*tolerance*/) {
        for (const resbuf* rb = pXData; rb != nullptr; rb = rb->rbnext) {
            if (rb->restype == kDxfXdInteger16 && rb->resval.rint == expectedValue) {
                return true;
            }
            if (rb->restype == kDxfXdInteger32 && rb->resval.rlong == static_cast<std::int32_t>(expectedValue)) {
                return true;
            }
        }
        return false;
    }

    /// @brief 匹配 XData 值（浮点数）
    [[nodiscard]] static bool matchXDataValue(const resbuf* pXData, double expectedValue, double tolerance) {
        for (const resbuf* rb = pXData; rb != nullptr; rb = rb->rbnext) {
            if (rb->restype == kDxfXdReal) {
                if (std::abs(rb->resval.rreal - expectedValue) < tolerance) {
                    return true;
                }
            }
        }
        return false;
    }
};

/// @brief 便捷的工厂函数：创建 OR 逻辑的多字符串值匹配
inline auto makeXDataAny(const CString& appName, std::initializer_list<CString> values) {
    std::vector<XDataCondition> conditions;
    for (const auto& value : values) {
        conditions.emplace_back(appName, value);
    }
    return std::make_unique<XDataSpecification>(conditions, XDataSpecification::MatchMode::Any);
}

/// @brief 便捷的工厂函数：创建 AND 逻辑的多条件匹配
inline auto makeXDataAll(std::initializer_list<XDataCondition> conditions) { return std::make_unique<XDataSpecification>(std::vector<XDataCondition>(conditions), XDataSpecification::MatchMode::All); }

// ============================================
// BasicFilter 针对 ARX/ZRX 类型的特化
// ============================================

/// @brief 针对 AcDbObjectId 的 BasicFilter 特化，支持 AcDbObjectIdArray
template <>
struct BasicFilter<AcDbObjectId> : Filter<AcDbObjectId> {
    [[nodiscard]] std::vector<AcDbObjectId> filter(const std::vector<AcDbObjectId>& items, const Specification<AcDbObjectId>& spec) const override {
        std::vector<AcDbObjectId> res;
        res.reserve(items.size());
        for (const auto& item : items) {
            if (spec.is_satisfied(item)) {
                res.push_back(item);
            }
        }
        return res;
    }

    /// @brief 针对 AcDbObjectIdArray 的重载（返回 vector）
    /// @param items AcDbObjectIdArray 数组
    /// @param spec 过滤规约
    /// @return 过滤后的结果 vector
    [[nodiscard]] static std::vector<AcDbObjectId> filter(const AcDbObjectIdArray& items, const Specification<AcDbObjectId>& spec) {
        std::vector<AcDbObjectId> res;
        res.reserve(items.logicalLength());
        for (int i = 0; i < items.logicalLength(); ++i) {
            const auto& item = items[i];
            if (spec.is_satisfied(item)) {
                res.push_back(item);
            }
        }
        return res;
    }

    /// @brief 针对 AcDbObjectIdArray 的重载（返回 AcDbObjectIdArray）
    /// @param items AcDbObjectIdArray 数组
    /// @param spec 过滤规约
    /// @return 过滤后的结果 AcDbObjectIdArray
    [[nodiscard]] static AcDbObjectIdArray filterToArray(const AcDbObjectIdArray& items, const Specification<AcDbObjectId>& spec) {
        AcDbObjectIdArray result;
        for (int i = 0; i < items.logicalLength(); ++i) {
            const auto& item = items[i];
            if (spec.is_satisfied(item)) {
                result.append(item);
            }
        }
        return result;
    }
};