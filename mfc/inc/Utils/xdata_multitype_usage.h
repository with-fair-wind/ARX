#pragma once

#include <Collect/collect.h>
#include <Utils/zcBm_filter_builder.h>
#include <example/cadFilter.h>

/// @file xdata_multitype_usage.h
/// @brief XDataSpecification 多类型值匹配使用示例

namespace XDataMultiTypeUsageExamples {

// ============================================
// 示例 1: 字符串值匹配
// ============================================
inline void example1_StringMatch() {
    // 匹配 "标签" appname 下值为 "符号@引线符号" 的实体
    auto spec = std::make_unique<XDataSpecification>(L"标签", L"符号@引线符号");

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 2: 整数值匹配
// ============================================
inline void example2_IntMatch() {
    // 匹配 "层级" appname 下值为 1 的实体
    auto spec = std::make_unique<XDataSpecification>(L"层级", 1);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 3: 浮点数值匹配（带容差）
// ============================================
inline void example3_DoubleMatch() {
    // 匹配 "高度" appname 下值约为 3.14 的实体（容差 0.01）
    auto spec = std::make_unique<XDataSpecification>(L"高度", 3.14, 0.01);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 4: 混合类型 - OR 逻辑
// ============================================
inline void example4_MixedTypesOr() {
    // 匹配：标签="符号@电缆井" OR 层级=1 OR 高度≈2.5
    std::vector<XDataCondition> conditions = {
        {L"标签", L"符号@电缆井"}, {L"层级", 1}, {L"高度", 2.5, 0.1}  // 容差 0.1
    };
    auto spec = std::make_unique<XDataSpecification>(conditions, XDataSpecification::MatchMode::Any);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 5: 混合类型 - AND 逻辑
// ============================================
inline void example5_MixedTypesAnd() {
    // 匹配：标签="符号@电缆井" AND 层级=1 AND 高度≈3.5
    std::vector<XDataCondition> conditions = {{L"标签", L"符号@电缆井"}, {L"层级", 1}, {L"高度", 3.5, 0.05}};
    auto spec = std::make_unique<XDataSpecification>(conditions, XDataSpecification::MatchMode::All);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 6: 流式 API - 混合类型
// ============================================
inline void example6_FluentAPI() {
    // 使用流式 API 动态添加不同类型的条件
    auto spec = std::make_unique<XDataSpecification>(L"标签", L"符号@电缆井");
    spec->withInt(L"层级", 1).withDouble(L"高度", 3.5, 0.05).withString(L"状态", L"激活").setMatchMode(XDataSpecification::MatchMode::All);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 7: 多个整数值匹配（OR）
// ============================================
inline void example7_MultipleInts() {
    // 匹配层级为 1, 2 或 3 的实体
    std::vector<XDataCondition> conditions = {{L"层级", 1}, {L"层级", 2}, {L"层级", 3}};
    auto spec = std::make_unique<XDataSpecification>(conditions, XDataSpecification::MatchMode::Any);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 8: 范围匹配（使用多个浮点数条件）
// ============================================
inline void example8_RangeMatch() {
    // 匹配高度在 [2.5, 3.5] 范围内的实体（通过多个带容差的值）
    std::vector<XDataCondition> conditions = {{L"高度", 2.5, 0.01}, {L"高度", 3.0, 0.01}, {L"高度", 3.5, 0.01}};
    auto spec = std::make_unique<XDataSpecification>(conditions, XDataSpecification::MatchMode::Any);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 9: 结合 FilterBuilder 使用
// ============================================
inline void example9_WithFilterBuilder() {
    // 先用 FilterBuilder 过滤类型，再用 XDataSpecification 过滤 XData
    auto* resbufFilter = ZcBmFilterBuilder::And().types<AcDbBlockReference>().build();
    PolygenSelect polySelector(resbufFilter);

    // 多类型条件：标签是字符串，层级是整数
    std::vector<XDataCondition> conditions = {{L"标签", L"符号@电缆井"}, {L"层级", 1}};
    auto xdataSpec = std::make_unique<XDataSpecification>(conditions, XDataSpecification::MatchMode::All);
    FilterCollect collector(polySelector, std::move(xdataSpec));

    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);

    acutRelRb(resbufFilter);
}

// ============================================
// 示例 10: 复杂条件 - 多类型 + 多层过滤
// ============================================
inline void example10_Complex() {
    // 1. 先用 resbuf 过滤块参照类型
    auto* resbufFilter = ZcBmFilterBuilder::And().types<AcDbBlockReference>().build();
    AllSelect allSelector(resbufFilter);

    // 2. 多类型条件：字符串 + 整数 + 浮点数（OR 逻辑）
    std::vector<XDataCondition> conditions = {
        {L"标签", L"符号@电缆井"},    // 字符串
        {L"标签", L"符号@引线符号"},  // 字符串
        {L"层级", 1},                 // 整数
        {L"层级", 2},                 // 整数
        {L"高度", 3.5, 0.1}           // 浮点数（容差 0.1）
    };
    auto spec1 = std::make_unique<XDataSpecification>(conditions, XDataSpecification::MatchMode::Any);
    FilterCollect collector1(allSelector, std::move(spec1));

    AcDbObjectIdArray ids;
    collector1.collectEntIds(ids);

    acutRelRb(resbufFilter);

    acutPrintf(_T("\n找到 %d 个符合条件的实体"), ids.logicalLength());
}

// ============================================
// 示例 11: 精确整数匹配（支持 int16 和 int32）
// ============================================
inline void example11_IntegerTypes() {
    // XData 可能存储为 kDxfXdInteger16 或 kDxfXdInteger32
    // 我们的实现自动支持两种类型
    auto spec = std::make_unique<XDataSpecification>(L"ID", 12345);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

// ============================================
// 示例 12: 高精度浮点数匹配
// ============================================
inline void example12_HighPrecisionDouble() {
    // 匹配高精度浮点数（容差 1e-9）
    auto spec = std::make_unique<XDataSpecification>(L"PI", 3.141592653589793, 1e-9);

    AllSelect allSelector;
    FilterCollect collector(allSelector, std::move(spec));
    AcDbObjectIdArray ids;
    collector.collectEntIds(ids);
}

}  // namespace XDataMultiTypeUsageExamples
