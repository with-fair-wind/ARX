#include <Utils/zcBm_filter_builder.h>

namespace {
void zcBmEmitCtrl(ZcBmFilterOp filterOp, bool begin, ZcBmResbufChain& out) {
    const ACHAR* ctrlText = nullptr;
    switch (filterOp) {
        case ZcBmFilterOp::And:
            ctrlText = begin ? L"<AND" : L"AND>";
            break;
        case ZcBmFilterOp::Or:
            ctrlText = begin ? L"<OR" : L"OR>";
            break;
        case ZcBmFilterOp::Not:
            ctrlText = begin ? L"<NOT" : L"NOT>";
            break;
        default:
            return;
    }

    ZcBmResbufChain chain;
    if (!ZcBmResbufCodec<std::wstring>::encode(kDxfOperator, std::wstring(ctrlText), &chain)) {
        chain.release();
        return;
    }
    out.append(chain.detach());
}
}  // namespace

ZcBmFilterLeaf::ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, const ACHAR* stringValue)
    : ZcBmFilterNode(ZcBmFilterOp::Leaf), m_dxfCode(dxfCode.value()), m_restype(RTSTR), m_stringValue(stringValue == nullptr ? L"" : stringValue) {}

ZcBmFilterLeaf::ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, const std::wstring& stringValue)
    : ZcBmFilterNode(ZcBmFilterOp::Leaf), m_dxfCode(dxfCode.value()), m_restype(RTSTR), m_stringValue(stringValue) {}

ZcBmFilterLeaf::ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, std::int16_t int16Value)
    : ZcBmFilterNode(ZcBmFilterOp::Leaf), m_dxfCode(dxfCode.value()), m_restype(RTSHORT), m_int16Value(int16Value) {}

ZcBmFilterLeaf::ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, std::int32_t int32Value)
    : ZcBmFilterNode(ZcBmFilterOp::Leaf), m_dxfCode(dxfCode.value()), m_restype(RTLONG), m_int32Value(int32Value) {}

ZcBmFilterLeaf::ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, std::int64_t int64Value)
    : ZcBmFilterNode(ZcBmFilterOp::Leaf), m_dxfCode(dxfCode.value()), m_restype(RTINT64), m_int64Value(int64Value) {}

ZcBmFilterLeaf::ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, const zds_name& nameValue) : ZcBmFilterNode(ZcBmFilterOp::Leaf), m_dxfCode(dxfCode.value()), m_restype(RTENAME) {
    m_nameValue[0] = nameValue[0];
    m_nameValue[1] = nameValue[1];
}

ZcBmFilterLeaf::ZcBmFilterLeaf(ZcBmDxfGroupCode dxfCode, double doubleValue)
    : ZcBmFilterNode(ZcBmFilterOp::Leaf), m_dxfCode(dxfCode.value()), m_restype(RTREAL), m_doubleValue(doubleValue) {}

void ZcBmFilterLeaf::emit(ZcBmResbufChain& out) const {
    switch (m_restype) {
        case RTSTR:
            ZcBmResbufCodec<std::wstring>::encode(m_dxfCode, m_stringValue, &out);
            break;
        case RTSHORT:
            ZcBmResbufCodec<std::int16_t>::encode(m_dxfCode, m_int16Value, &out);
            break;
        case RTLONG:
            ZcBmResbufCodec<std::int32_t>::encode(m_dxfCode, m_int32Value, &out);
            break;
        case RTINT64:
            ZcBmResbufCodec<std::int64_t>::encode(m_dxfCode, m_int64Value, &out);
            break;
        case RTENAME:
            ZcBmResbufCodec<zds_name>::encode(m_dxfCode, m_nameValue, &out);
            break;
        case RTREAL:
            ZcBmResbufCodec<double>::encode(m_dxfCode, m_doubleValue, &out);
            break;
        default:
            break;
    }
}

ZcBmFilterXDataNode::ZcBmFilterXDataNode(const ACHAR* appName) : ZcBmFilterNode(ZcBmFilterOp::XData), m_appName(appName == nullptr ? L"" : appName) {}

void ZcBmFilterXDataNode::addString(const ACHAR* value) { m_xdataItems.push_back(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfXdAsciiString}, value)); }

void ZcBmFilterXDataNode::addReal(double value) { m_xdataItems.push_back(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfXdReal}, value)); }

void ZcBmFilterXDataNode::addShort(std::int16_t value) { m_xdataItems.push_back(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfXdInteger16}, value)); }

void ZcBmFilterXDataNode::addLong(std::int32_t value) { m_xdataItems.push_back(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfXdInteger32}, value)); }

void ZcBmFilterXDataNode::emit(ZcBmResbufChain& out) const {
    ZcBmResbufCodec<ZcBmNilTag>::encode(kDxfXDataStart, ZcBmNilTag{}, &out);
    ZcBmResbufCodec<std::wstring>::encode(kDxfRegAppName, m_appName, &out);

    for (const auto& item : m_xdataItems) {
        item->emit(out);
    }
}

ZcBmFilterLogicNode::ZcBmFilterLogicNode(ZcBmFilterOp filterOp) : ZcBmFilterNode(filterOp) {}

void ZcBmFilterLogicNode::addChild(std::unique_ptr<ZcBmFilterNode> child) { m_children.emplace_back(std::move(child)); }

void ZcBmFilterLogicNode::emit(ZcBmResbufChain& out) const {
    zcBmEmitCtrl(op(), true, out);
    for (const auto& child : m_children) {
        child->emit(out);
    }
    zcBmEmitCtrl(op(), false, out);
}

ZcBmFilterBuilder::ZcBmFilterBuilder(ZcBmFilterOp filterOp) { m_root = std::make_unique<ZcBmFilterLogicNode>(filterOp); }

ZcBmFilterBuilder ZcBmFilterBuilder::Direct() { return ZcBmFilterBuilder(ZcBmFilterOp::Leaf); }
ZcBmFilterBuilder ZcBmFilterBuilder::And() { return ZcBmFilterBuilder(ZcBmFilterOp::And); }
ZcBmFilterBuilder ZcBmFilterBuilder::Or() { return ZcBmFilterBuilder(ZcBmFilterOp::Or); }
ZcBmFilterBuilder ZcBmFilterBuilder::Not() { return ZcBmFilterBuilder(ZcBmFilterOp::Not); }

ZcBmFilterBuilder& ZcBmFilterBuilder::type(AcRxClass* cls) {
    if (cls != nullptr && cls->dxfName() != nullptr) {
        static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{RTDXF0}, cls->dxfName()));
    }
    return *this;
}

ZcBmFilterBuilder& ZcBmFilterBuilder::types(const std::vector<AcRxClass*>& classes) {
    for (auto* cls : classes) {
        type(cls);
    }
    return *this;
}

ZcBmFilterBuilder& ZcBmFilterBuilder::layer(const ACHAR* name) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfLayerName}, name));
    return *this;
}
ZcBmFilterBuilder& ZcBmFilterBuilder::block(const ACHAR* name) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfBlockName}, name));
    return *this;
}
ZcBmFilterBuilder& ZcBmFilterBuilder::linetype(const ACHAR* name) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfLinetypeName}, name));
    return *this;
}
ZcBmFilterBuilder& ZcBmFilterBuilder::color(int aci) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfColor}, static_cast<std::int16_t>(aci)));
    return *this;
}
ZcBmFilterBuilder& ZcBmFilterBuilder::trueColor(int rgb) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfColorRGB}, static_cast<std::int32_t>(rgb)));
    return *this;
}
ZcBmFilterBuilder& ZcBmFilterBuilder::lineWeight(int lineWeight) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfLineWeight}, static_cast<std::int16_t>(lineWeight)));
    return *this;
}
ZcBmFilterBuilder& ZcBmFilterBuilder::visible(bool isVisible) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfVisibility}, static_cast<std::int16_t>(isVisible ? 0 : 1)));
    return *this;
}
ZcBmFilterBuilder& ZcBmFilterBuilder::handle(const ACHAR* handleValue) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::make_unique<ZcBmFilterLeaf>(ZcBmDxfGroupCode{kDxfHandle}, handleValue));
    return *this;
}

ZcBmFilterBuilder& ZcBmFilterBuilder::xdata(const ACHAR* appName, const std::function<void(ZcBmXDataBuilder&)>& buildFunc) {
    auto xdataNode = std::make_unique<ZcBmFilterXDataNode>(appName);
    ZcBmXDataBuilder builder(*xdataNode);
    buildFunc(builder);
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::move(xdataNode));
    return *this;
}

ZcBmFilterBuilder& ZcBmFilterBuilder::add(ZcBmFilterBuilder&& child) {
    static_cast<ZcBmFilterLogicNode*>(m_root.get())->addChild(std::move(child.m_root));
    return *this;
}

ZcBmFilterBuilder ZcBmFilterBuilder::negate() {
    ZcBmFilterBuilder notBuilder(ZcBmFilterOp::Not);
    notBuilder.add(std::move(*this));
    return notBuilder;
}

resbuf* ZcBmFilterBuilder::build() const {
    ZcBmResbufChain out;
    m_root->emit(out);
    return out.detach();
}
