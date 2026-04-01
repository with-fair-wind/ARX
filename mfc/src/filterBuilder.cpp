#include <Utils/filterBuilder.h>

// ============================================================
// helpers
// ============================================================

void ResbufChain::append(resbuf* node) noexcept {
    if (node == nullptr) {
        return;
    }
    if (m_head == nullptr) {
        m_head = node;
        m_tail = node;
        return;
    }

    m_tail->rbnext = node;
    m_tail = node;
}

namespace {
void emitCtrl(FilterOp filterOp, bool begin, ResbufChain& out) {
    const ACHAR* ctrlText = nullptr;
    switch (filterOp) {
        case FilterOp::And:
            ctrlText = begin ? L"<AND" : L"AND>";
            break;
        case FilterOp::Or:
            ctrlText = begin ? L"<OR" : L"OR>";
            break;
        case FilterOp::Not:
            ctrlText = begin ? L"<NOT" : L"NOT>";
            break;
        default:
            return;
    }

    resbuf* ctrlRb = acutNewRb(kDxfOperator);
    acutNewString(ctrlText, ctrlRb->resval.rstring);
    out.append(ctrlRb);
}
}  // namespace

// ============================================================
// Leaf
// ============================================================

FilterLeaf::FilterLeaf(DxfGroupCode dxfCode, const ACHAR* stringValue) : FilterNode(FilterOp::Leaf), m_dxfCode(dxfCode.value()), m_valueType(ValueType::String) {
    m_value.stringValue = nullptr;
    acutNewString(stringValue, m_value.stringValue);
}

FilterLeaf::FilterLeaf(DxfGroupCode dxfCode, std::int16_t int16Value) : FilterNode(FilterOp::Leaf), m_dxfCode(dxfCode.value()) { m_value.int16Value = int16Value; }

FilterLeaf::FilterLeaf(DxfGroupCode dxfCode, std::int32_t int32Value) : FilterNode(FilterOp::Leaf), m_dxfCode(dxfCode.value()), m_valueType(ValueType::Int32) { m_value.int32Value = int32Value; }

FilterLeaf::FilterLeaf(DxfGroupCode dxfCode, double doubleValue) : FilterNode(FilterOp::Leaf), m_dxfCode(dxfCode.value()), m_valueType(ValueType::Double) { m_value.doubleValue = doubleValue; }

FilterLeaf::~FilterLeaf() {
    if (m_valueType == ValueType::String && m_value.stringValue != nullptr) {
        acutDelString(m_value.stringValue);
        m_value.stringValue = nullptr;
    }
}

void FilterLeaf::emit(ResbufChain& out) const {
    resbuf* leafRb = acutNewRb(m_dxfCode);

    switch (m_valueType) {
        case ValueType::String:
            acutNewString(m_value.stringValue, leafRb->resval.rstring);
            break;
        case ValueType::Int16:
            leafRb->resval.rint = m_value.int16Value;
            break;
        case ValueType::Int32:
            leafRb->resval.rlong = m_value.int32Value;
            break;
        case ValueType::Double:
            leafRb->resval.rreal = m_value.doubleValue;
    }
    out.append(leafRb);
}

// ============================================================
// XData
// ============================================================

FilterXDataNode::FilterXDataNode(const ACHAR* appName) : FilterNode(FilterOp::XData) {
    m_appName = nullptr;
    if (appName != nullptr) {
        acutNewString(appName, m_appName);
    }
}

FilterXDataNode::~FilterXDataNode() {
    if (m_appName != nullptr) {
        acutDelString(m_appName);
        m_appName = nullptr;
    }
}

void FilterXDataNode::addString(const ACHAR* value) { m_xdataItems.push_back(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfXdAsciiString}, value)); }

void FilterXDataNode::addReal(double value) { m_xdataItems.push_back(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfXdReal}, value)); }

void FilterXDataNode::addShort(std::int16_t value) { m_xdataItems.push_back(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfXdInteger16}, value)); }

void FilterXDataNode::emit(ResbufChain& out) const {
    // AutoCAD/ZWCAD XData 过滤器的正确格式（在 resbuf 链表中）：
    // (-3)                    ; 标记 XData 过滤开始
    // (1001 . "应用名")        ; 应用名
    // (1000 . "字符串值")      ; XData 字符串值（可选）
    // (1040 . 实数值)          ; XData 实数值（可选）
    // ...
    //
    // 注意：ZWCAD 的 resbuf 不支持 rbuf 子链表，需使用“拍平”格式。

    // 1. 创建 -3 标记（XData 过滤开始）
    resbuf* minus3Rb = acutNewRb(kDxfXDataStart);
    out.append(minus3Rb);

    // 2. 添加应用名 (1001)
    resbuf* appNameRb = acutNewRb(kDxfRegAppName);
    acutNewString(m_appName, appNameRb->resval.rstring);
    out.append(appNameRb);

    // 3. 添加 XData 项（1000, 1040, 1070 等）
    for (const auto& item : m_xdataItems) {
        ResbufChain itemChain;
        item->emit(itemChain);
        if (itemChain.head() != nullptr) {
            out.append(itemChain.head());
        }
    }
}

// ============================================================
// Logic
// ============================================================

FilterLogicNode::FilterLogicNode(FilterOp filterOp) : FilterNode(filterOp) {}

void FilterLogicNode::addChild(std::unique_ptr<FilterNode> child) { m_children.emplace_back(std::move(child)); }

void FilterLogicNode::emit(ResbufChain& out) const {
    // 选择集过滤器的逻辑表达式需要用 -4 组码的控制串来保证语义不被“拍平”。
    // 尤其是 <OR 中如果不显式包一层 <AND ... AND>，会把 (A AND B) 当成 A OR B。
    bool wrap = true;

    if (wrap) {
        emitCtrl(op(), true, out);
    }

    for (const auto& child : m_children) {
        child->emit(out);
    }

    if (wrap) {
        emitCtrl(op(), false, out);
    }
}

// ============================================================
// Builder
// ============================================================

FilterBuilder::FilterBuilder(FilterOp filterOp) { root = std::make_unique<FilterLogicNode>(filterOp); }

FilterBuilder FilterBuilder::And() { return FilterBuilder(FilterOp::And); }
FilterBuilder FilterBuilder::Or() { return FilterBuilder(FilterOp::Or); }
FilterBuilder FilterBuilder::Not() { return FilterBuilder(FilterOp::Not); }

FilterBuilder& FilterBuilder::type(AcRxClass* cls) {
    if (cls != nullptr && cls->dxfName() != nullptr) {
        static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{RTDXF0}, cls->dxfName()));
    }
    return *this;
}

FilterBuilder& FilterBuilder::types(const std::vector<AcRxClass*>& classes) {
    for (auto* cls : classes) {
        type(cls);
    }
    return *this;
}

FilterBuilder& FilterBuilder::layer(const ACHAR* name) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfLayerName}, name));
    return *this;
}
FilterBuilder& FilterBuilder::block(const ACHAR* name) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfBlockName}, name));
    return *this;
}
FilterBuilder& FilterBuilder::linetype(const ACHAR* name) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfLinetypeName}, name));
    return *this;
}
FilterBuilder& FilterBuilder::color(int aci) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfColor}, static_cast<std::int16_t>(aci)));
    return *this;
}
FilterBuilder& FilterBuilder::trueColor(int rgb) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfColorRGB}, static_cast<std::int32_t>(rgb)));
    return *this;
}
FilterBuilder& FilterBuilder::lineWeight(int lineWeight) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfLineWeight}, static_cast<std::int16_t>(lineWeight)));
    return *this;
}
FilterBuilder& FilterBuilder::visible(bool isVisible) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfVisibility}, static_cast<std::int16_t>(isVisible ? 0 : 1)));
    return *this;
}
FilterBuilder& FilterBuilder::handle(const ACHAR* handleValue) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfHandle}, handleValue));
    return *this;
}

FilterBuilder& FilterBuilder::xdataApp(const ACHAR* app) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfRegAppName}, app));
    return *this;
}
FilterBuilder& FilterBuilder::xdataStr(const ACHAR* value) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfXdAsciiString}, value));
    return *this;
}
FilterBuilder& FilterBuilder::xdataReal(double value) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfXdReal}, value));
    return *this;
}
FilterBuilder& FilterBuilder::xdataShort(std::int16_t value) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::make_unique<FilterLeaf>(DxfGroupCode{kDxfXdInteger16}, value));
    return *this;
}

FilterBuilder& FilterBuilder::xdata(const ACHAR* appName, const std::function<void(XDataBuilder&)>& buildFunc) {
    auto xdataNode = std::make_unique<FilterXDataNode>(appName);
    XDataBuilder builder(*xdataNode);
    buildFunc(builder);
    static_cast<FilterLogicNode*>(root.get())->addChild(std::move(xdataNode));
    return *this;
}

FilterBuilder& FilterBuilder::add(FilterBuilder&& child) {
    static_cast<FilterLogicNode*>(root.get())->addChild(std::move(child.root));
    return *this;
}

FilterBuilder FilterBuilder::not_() {
    FilterBuilder notBuilder(FilterOp::Not);
    notBuilder.add(std::move(*this));
    return notBuilder;
}

resbuf* FilterBuilder::build() const {
    ResbufChain out;
    root->emit(out);
    return out.head();
}
