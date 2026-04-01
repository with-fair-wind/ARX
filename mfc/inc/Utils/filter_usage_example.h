#pragma once
#include <Func/filter.h>

#include <string>

// ============================================
// 使用示例：展示如何使用泛型过滤器框架
// ============================================

namespace FilterExample {

// 示例1: 整数过滤器 ====================

struct IsPositive : Specification<int> {
    [[nodiscard]] bool is_satisfied(const int& item) const override { return item > 0; }
};

struct IsEven : Specification<int> {
    [[nodiscard]] bool is_satisfied(const int& item) const override { return item % 2 == 0; }
};

struct IsLessThan : Specification<int> {
    explicit IsLessThan(int threshold) : threshold_(threshold) {}
    [[nodiscard]] bool is_satisfied(const int& item) const override { return item < threshold_; }

   private:
    int threshold_;
};

// 使用方式1: 组合多个条件（使用构造函数）
inline void example_int_filter_constructor() {
    IsPositive positive;
    IsEven even;
    IsLessThan less100(100);

    // 组合3个条件：正数 && 偶数 && 小于100
    AndSpecification<int> spec(positive, even, less100);

    BasicFilter<int> filter;
    std::vector<int> numbers = {-5, 2, 15, 20, 50, 75, 100, 101};
    auto result = filter.filter(numbers, spec);
    // result = {2, 20, 50}
}

// 使用方式2: 组合多个条件（使用辅助函数）
inline void example_int_filter_helper() {
    IsPositive positive;
    IsEven even;
    IsLessThan less100(100);

    // 使用 make_and/make_or 辅助函数
    auto andSpec = make_and<int>(positive, even, less100);
    auto orSpec = make_or<int>(positive, even);

    BasicFilter<int> filter;
    std::vector<int> numbers = {-5, 2, 15, 20, 50, 75, 100, 101};
    auto result1 = filter.filter(numbers, andSpec);
    auto result2 = filter.filter(numbers, orSpec);
}

// 示例2: 字符串过滤器 ====================

struct StringStartsWith : Specification<std::string> {
    explicit StringStartsWith(std::string prefix) : prefix_(std::move(prefix)) {}
    [[nodiscard]] bool is_satisfied(const std::string& item) const override { return item.find(prefix_) == 0; }

   private:
    std::string prefix_;
};

struct StringLengthGreaterThan : Specification<std::string> {
    explicit StringLengthGreaterThan(size_t length) : length_(length) {}
    [[nodiscard]] bool is_satisfied(const std::string& item) const override { return item.length() > length_; }

   private:
    size_t length_;
};

inline void example_string_filter() {
    StringStartsWith startsWithA("A");
    StringLengthGreaterThan longerThan5(5);

    // 组合条件：以"A"开头 && 长度>5
    AndSpecification<std::string> spec(startsWithA, longerThan5);

    BasicFilter<std::string> filter;
    std::vector<std::string> words = {"Apple", "Ant", "Banana", "Application", "A"};
    auto result = filter.filter(words, spec);
    // result = {"Application"}
}

// 示例3: AcDbObjectId 过滤器（原有功能） ====================
// 注意：如果要编译 CAD 相关示例，请在编译前定义 _ZWCAD 宏，并包含相应的 ZWCAD 头文件
#ifdef _ZWCAD  // 假设 AcDbObjectId 定义在 ZWCAD 环境中

struct IsLineEntity : Specification<AcDbObjectId> {
    [[nodiscard]] bool is_satisfied(const AcDbObjectId& item) const override {
        AcDbEntity* pEnt = nullptr;
        if (acdbOpenObject(pEnt, item, AcDb::kForRead) != Acad::eOk) {
            return false;
        }
        bool result = pEnt->isKindOf(AcDbLine::desc());
        pEnt->close();
        return result;
    }
};

struct IsOnLayer : Specification<AcDbObjectId> {
    explicit IsOnLayer(const ACHAR* layerName) : layerName_(layerName) {}
    [[nodiscard]] bool is_satisfied(const AcDbObjectId& item) const override {
        AcDbEntity* pEnt = nullptr;
        if (acdbOpenObject(pEnt, item, AcDb::kForRead) != Acad::eOk) {
            return false;
        }
        AcString layer = pEnt->layer();
        pEnt->close();
        return layer == layerName_;
    }

   private:
    AcString layerName_;
};

struct IsColorIndex : Specification<AcDbObjectId> {
    explicit IsColorIndex(Adesk::UInt16 colorIndex) : colorIndex_(colorIndex) {}
    [[nodiscard]] bool is_satisfied(const AcDbObjectId& item) const override {
        AcDbEntity* pEnt = nullptr;
        if (acdbOpenObject(pEnt, item, AcDb::kForRead) != Acad::eOk) {
            return false;
        }
        Adesk::UInt16 color = pEnt->colorIndex();
        pEnt->close();
        return color == colorIndex_;
    }

   private:
    Adesk::UInt16 colorIndex_;
};

// 使用方式：组合多个条件过滤 CAD 实体
inline void example_autocad_filter() {
    IsLineEntity isLine;
    IsOnLayer onLayer0(L"0");
    IsColorIndex redColor(1);

    // 组合条件：是直线 && 在图层"0"上 && 颜色是红色
    AndSpecification<AcDbObjectId> spec(isLine, onLayer0, redColor);

    BasicFilter<AcDbObjectId> filter;

    // 从模型空间获取所有实体
    AcDbBlockTable* pBlockTable = nullptr;
    acdbHostApplicationServices()->workingDatabase()->getBlockTable(pBlockTable, AcDb::kForRead);
    AcDbBlockTableRecord* pModelSpace = nullptr;
    pBlockTable->getAt(ACDB_MODEL_SPACE, pModelSpace, AcDb::kForRead);
    pBlockTable->close();

    AcDbBlockTableRecordIterator* pIter = nullptr;
    pModelSpace->newIterator(pIter);

    std::vector<AcDbObjectId> allEntities;
    for (pIter->start(); !pIter->done(); pIter->step()) {
        AcDbObjectId id;
        pIter->getEntityId(id);
        allEntities.push_back(id);
    }
    delete pIter;
    pModelSpace->close();

    // 过滤符合条件的实体
    auto result = filter.filter(allEntities, spec);
    // result 包含所有红色的、在图层"0"上的直线
}

// 示例4: 使用 OR 和 NOT 组合 ====================

inline void example_complex_combination() {
    IsLineEntity isLine;
    IsOnLayer onLayer0(L"0");
    IsOnLayer onLayerWall(L"WALL");
    IsColorIndex redColor(1);
    IsColorIndex blueColor(5);

    // 复杂条件1：(图层0 || 图层WALL) && (红色 || 蓝色)
    OrSpecification<AcDbObjectId> layerSpec(onLayer0, onLayerWall);
    OrSpecification<AcDbObjectId> colorSpec(redColor, blueColor);
    AndSpecification<AcDbObjectId> finalSpec(layerSpec, colorSpec);

    // 复杂条件2：是直线 && !(红色)
    NotSpecification<AcDbObjectId> notRed(redColor);
    AndSpecification<AcDbObjectId> lineNotRedSpec(isLine, notRed);

    BasicFilter<AcDbObjectId> filter;
    std::vector<AcDbObjectId> entities;  // 假设已填充

    auto result1 = filter.filter(entities, finalSpec);
    auto result2 = filter.filter(entities, lineNotRedSpec);
}

// 示例5: 支持5个以上的条件组合 ====================

inline void example_many_conditions() {
    IsPositive positive;
    IsEven even;
    IsLessThan less100(100);
    IsLessThan less50(50);
    IsLessThan less25(25);

    // 组合5个条件
    AndSpecification<int> spec(positive, even, less100, less50, less25);

    BasicFilter<int> filter;
    std::vector<int> numbers = {1, 2, 4, 8, 16, 24, 30, 50, 100};
    auto result = filter.filter(numbers, spec);
    // result = {2, 4, 8, 16, 24}
}

#endif  // _ZWSOFT_DEFINED

}  // namespace FilterExample
