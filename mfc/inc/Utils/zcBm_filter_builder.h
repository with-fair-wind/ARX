#pragma once

#include <Utils/zcBm_resbuf_codec.h>
#include <acdb.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace zcbm_detail {
template <typename T>
struct IsSupportedMatchValue;
}

enum class ZcBmFilterOp : std::uint8_t { And, Or, Not, Leaf, XData };

class ZcBmFilterNode {
   public:
    virtual ~ZcBmFilterNode() = default;

    ZcBmFilterOp op() const noexcept { return m_op; }
    virtual void emit(ZcBmResbufChain& out) const = 0;

   protected:
    explicit ZcBmFilterNode(ZcBmFilterOp filterOp) : m_op(filterOp) {}

   private:
    ZcBmFilterOp m_op;
};

class ZcBmDxfGroupCode {
   public:
    explicit constexpr ZcBmDxfGroupCode(int value) noexcept : m_value(value) {}
    constexpr int value() const noexcept { return m_value; }

   private:
    int m_value;
};

class ZcBmFilterLeaf final : public ZcBmFilterNode {
   public:
    ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, const ACHAR* stringValue);
    ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, const std::wstring& stringValue);
    ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, std::int16_t int16Value);
    ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, std::int32_t int32Value);
    ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, std::int64_t int64Value);
    ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, const zds_name& nameValue);
    ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, double doubleValue);

    void emit(ZcBmResbufChain& out) const override;

   private:
    int m_dxfCode = 0;
    short m_restype = RTSHORT;
    std::int16_t m_int16Value = 0;
    std::int32_t m_int32Value = 0;
    std::int64_t m_int64Value = 0;
    zds_name m_nameValue = {0, 0};
    double m_doubleValue = 0.0;
    std::wstring m_stringValue;
};

class ZcBmFilterXDataNode final : public ZcBmFilterNode {
   public:
    explicit ZcBmFilterXDataNode(const ACHAR* appName);
    ~ZcBmFilterXDataNode() override = default;

    void addString(const ACHAR* value);
    void addReal(double value);
    void addShort(std::int16_t value);
    void addLong(std::int32_t value);
    void emit(ZcBmResbufChain& out) const override;

   private:
    std::wstring m_appName;
    std::vector<std::unique_ptr<ZcBmFilterLeaf>> m_xdataItems;
};

class ZcBmFilterLogicNode final : public ZcBmFilterNode {
   public:
    explicit ZcBmFilterLogicNode(ZcBmFilterOp filterOp);
    void addChild(std::unique_ptr<ZcBmFilterNode> child);
    void emit(ZcBmResbufChain& out) const override;

   private:
    std::vector<std::unique_ptr<ZcBmFilterNode>> m_children;
};

class ZcBmFilterBuilder {
   public:
    // Build a plain condition chain without <AND/<OR/<NOT control operators.
    static ZcBmFilterBuilder Direct();
    static ZcBmFilterBuilder And();
    static ZcBmFilterBuilder Or();
    static ZcBmFilterBuilder Not();

    ZcBmFilterBuilder& type(AcRxClass* cls);

    template <typename... Ts>
    ZcBmFilterBuilder& types();

    ZcBmFilterBuilder& types(const std::vector<AcRxClass*>& classes);

    ZcBmFilterBuilder& layer(const ACHAR* name);
    ZcBmFilterBuilder& block(const ACHAR* name);
    ZcBmFilterBuilder& linetype(const ACHAR* name);
    ZcBmFilterBuilder& color(int aci);
    ZcBmFilterBuilder& trueColor(int rgb);
    ZcBmFilterBuilder& lineWeight(int lineWeight);
    ZcBmFilterBuilder& visible(bool isVisible);
    ZcBmFilterBuilder& handle(const ACHAR* handleValue);
    template <typename T, typename std::enable_if<zcbm_detail::IsSupportedMatchValue<typename std::decay<T>::type>::value, int>::type = 0>
    ZcBmFilterBuilder& match(ZcBmDxfGroupCode dxfCode, T&& value);

    ZcBmFilterBuilder& xdata(const ACHAR* appName, const std::function<void(class ZcBmXDataBuilder&)>& buildFunc);

    ZcBmFilterBuilder& add(ZcBmFilterBuilder&& child);
    ZcBmFilterBuilder negate();

    resbuf* build() const;

   private:
    explicit ZcBmFilterBuilder(ZcBmFilterOp filterOp);
    std::unique_ptr<ZcBmFilterNode> m_root;
};

class ZcBmXDataBuilder {
   public:
    explicit ZcBmXDataBuilder(ZcBmFilterXDataNode& node) : m_node(node) {}

    ZcBmXDataBuilder& str(const ACHAR* value) {
        m_node.get().addString(value);
        return *this;
    }
    ZcBmXDataBuilder& real(double value) {
        m_node.get().addReal(value);
        return *this;
    }
    ZcBmXDataBuilder& shortInt(std::int16_t value) {
        m_node.get().addShort(value);
        return *this;
    }
    ZcBmXDataBuilder& longInt(std::int32_t value) {
        m_node.get().addLong(value);
        return *this;
    }

   private:
    std::reference_wrapper<ZcBmFilterXDataNode> m_node;
};

namespace zcbm_detail {
template <typename T>
struct IsSupportedMatchValue : std::false_type {};

template <>
struct IsSupportedMatchValue<const ACHAR*> : std::true_type {};
template <>
struct IsSupportedMatchValue<ACHAR*> : std::true_type {};
template <>
struct IsSupportedMatchValue<std::wstring> : std::true_type {};
template <>
struct IsSupportedMatchValue<std::int16_t> : std::true_type {};
template <>
struct IsSupportedMatchValue<std::int32_t> : std::true_type {};
template <>
struct IsSupportedMatchValue<std::int64_t> : std::true_type {};
template <>
struct IsSupportedMatchValue<double> : std::true_type {};
template <>
struct IsSupportedMatchValue<zds_name> : std::true_type {};

template <typename T>
struct IsEntityDerived {
    static const bool value = std::is_base_of<AcDbEntity, T>::value;
};

template <typename... Ts>
struct AllEntityDerived;

template <>
struct AllEntityDerived<> {
    static const bool value = true;
};

template <typename T, typename... Ts>
struct AllEntityDerived<T, Ts...> {
    static const bool value = IsEntityDerived<T>::value && AllEntityDerived<Ts...>::value;
};
}  // namespace zcbm_detail

template <typename... Ts>
ZcBmFilterBuilder& ZcBmFilterBuilder::types() {
    static_assert(zcbm_detail::AllEntityDerived<Ts...>::value, "types<Ts...> requires AcDbEntity derived types");

    int unused[] = {0, (type(Ts::desc()), 0)...};
    (void)unused;
    return *this;
}

template <typename T, typename std::enable_if<zcbm_detail::IsSupportedMatchValue<typename std::decay<T>::type>::value, int>::type>
ZcBmFilterBuilder& ZcBmFilterBuilder::match(ZcBmDxfGroupCode dxfCode, T&& value) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(dxfCode, std::forward<T>(value)));
    return *this;
}
