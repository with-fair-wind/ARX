#pragma once

#include <functional>

// ============================================================
// AST
// ============================================================

enum class FilterOp : std::uint8_t { And, Or, Not, Leaf, XData };

class ResbufChain {
   public:
    void append(resbuf* node) noexcept;
    [[nodiscard]] resbuf* head() const noexcept { return m_head; }

   private:
    resbuf* m_head = nullptr;
    resbuf* m_tail = nullptr;
};

class FilterNode {
   public:
    virtual ~FilterNode() = default;

    [[nodiscard]] FilterOp op() const noexcept { return m_op; }
    virtual void emit(ResbufChain& out) const = 0;

   protected:
    explicit FilterNode(FilterOp filterOp) : m_op(filterOp) {}

   private:
    FilterOp m_op;
};

// ============================================================
// Leaf
// ============================================================

class DxfGroupCode {
   public:
    explicit constexpr DxfGroupCode(int value) noexcept : m_value(value) {}
    [[nodiscard]] constexpr int value() const noexcept { return m_value; }

   private:
    int m_value;
};

class FilterLeaf final : public FilterNode {
   public:
    enum class ValueType : std::uint8_t { Int16, Int32, Double, String };

    FilterLeaf(DxfGroupCode dxfCode, const ACHAR* stringValue);
    FilterLeaf(DxfGroupCode dxfCode, std::int16_t int16Value);
    FilterLeaf(DxfGroupCode dxfCode, std::int32_t int32Value);
    FilterLeaf(DxfGroupCode dxfCode, double doubleValue);
    ~FilterLeaf() override;

    FilterLeaf(const FilterLeaf&) = delete;
    FilterLeaf& operator=(const FilterLeaf&) = delete;
    FilterLeaf(FilterLeaf&&) = delete;
    FilterLeaf& operator=(FilterLeaf&&) = delete;

    void emit(ResbufChain& out) const override;

   private:
    union Value {
        std::int16_t int16Value;
        std::int32_t int32Value;
        double doubleValue;
        ACHAR* stringValue;
    };

    int m_dxfCode = 0;
    ValueType m_valueType = ValueType::Int16;
    Value m_value{};
};

// ============================================================
// XData
// ============================================================

/**
 * @brief XData 过滤节点，生成 (-3 ("应用名" (1000 . "值") ...)) 格式
 */
class FilterXDataNode final : public FilterNode {
   public:
    explicit FilterXDataNode(const ACHAR* appName);
    ~FilterXDataNode() override;

    FilterXDataNode(const FilterXDataNode&) = delete;
    FilterXDataNode& operator=(const FilterXDataNode&) = delete;
    FilterXDataNode(FilterXDataNode&&) = delete;
    FilterXDataNode& operator=(FilterXDataNode&&) = delete;

    void addString(const ACHAR* value);
    void addReal(double value);
    void addShort(std::int16_t value);
    void emit(ResbufChain& out) const override;

   private:
    ACHAR* m_appName = nullptr;
    std::vector<std::unique_ptr<FilterLeaf>> m_xdataItems;
};

// ============================================================
// Logic
// ============================================================

class FilterLogicNode final : public FilterNode {
   public:
    explicit FilterLogicNode(FilterOp filterOp);
    void addChild(std::unique_ptr<FilterNode> child);
    void emit(ResbufChain& out) const override;

   private:
    std::vector<std::unique_ptr<FilterNode>> m_children;
};

// ============================================================
// Builder
// ============================================================

class FilterBuilder {
   public:
    // ---------- factories ----------
    static FilterBuilder And();
    static FilterBuilder Or();
    static FilterBuilder Not();

    // ---------- entity type ----------
    FilterBuilder& type(AcRxClass* cls);

    template <typename... Ts>
    FilterBuilder& types();

    FilterBuilder& types(const std::vector<AcRxClass*>& classes);

    // ---------- common DXF ----------
    FilterBuilder& layer(const ACHAR* name);
    FilterBuilder& block(const ACHAR* name);
    FilterBuilder& linetype(const ACHAR* name);
    FilterBuilder& color(int aci);
    FilterBuilder& trueColor(int rgb);
    FilterBuilder& lineWeight(int lineWeight);
    FilterBuilder& visible(bool isVisible);
    FilterBuilder& handle(const ACHAR* handleValue);

    // ---------- XData (旧 API，不推荐使用，将被废弃) ----------
    [[deprecated("使用 xdata() 代替")]] FilterBuilder& xdataApp(const ACHAR* app);
    [[deprecated("使用 xdata() 代替")]] FilterBuilder& xdataStr(const ACHAR* value);
    [[deprecated("使用 xdata() 代替")]] FilterBuilder& xdataReal(double value);
    [[deprecated("使用 xdata() 代替")]] FilterBuilder& xdataShort(std::int16_t value);

    // ---------- XData (新 API，推荐使用) ----------
    /**
     * @brief 添加 XData 过滤条件，生成正确的 (-3 ...) 格式
     * @param appName XData 应用名
     * @param buildFunc 用于构建 XData 条件的回调函数
     * 
     * 使用示例：
     * @code
     * FilterBuilder::And()
     *     .types<AcDbBlockReference>()
     *     .xdata(L"标签", [](XDataBuilder& xd) {
     *         xd.str(L"符号@电缆井");
     *     })
     *     .build();
     * @endcode
     */
    class XDataBuilder;
    FilterBuilder& xdata(const ACHAR* appName, const std::function<void(XDataBuilder&)>& buildFunc);

    // ---------- composition ----------
    FilterBuilder& add(FilterBuilder&& child);
    FilterBuilder not_();

    // ---------- build ----------
    [[nodiscard]] resbuf* build() const;

   private:
    explicit FilterBuilder(FilterOp filterOp);
    std::unique_ptr<FilterNode> root;
};

// ============================================================
// XDataBuilder Helper
// ============================================================

/**
 * @brief XData 构建器辅助类
 */
class FilterBuilder::XDataBuilder {
   public:
    explicit XDataBuilder(FilterXDataNode& node) : m_node(node) {}

    XDataBuilder& str(const ACHAR* value) {
        m_node.get().addString(value);
        return *this;
    }

    XDataBuilder& real(double value) {
        m_node.get().addReal(value);
        return *this;
    }

    XDataBuilder& shortInt(std::int16_t value) {
        m_node.get().addShort(value);
        return *this;
    }

   private:
    std::reference_wrapper<FilterXDataNode> m_node;
};

// ============================================================
// template impl
// ============================================================

template <typename... Ts>
FilterBuilder& FilterBuilder::types() {
    static_assert((std::is_base_of_v<AcDbEntity, Ts> && ...), "types<Ts...> requires AcDbEntity derived types");

    (type(Ts::desc()), ...);
    return *this;
}
