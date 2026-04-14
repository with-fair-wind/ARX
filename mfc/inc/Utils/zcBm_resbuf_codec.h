#pragma once

#include <acedads.h>
#include <acutads.h>
#include <zAcString.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

struct ZcBmNilTag {};

class ZcBmResbufChain {
   public:
    ZcBmResbufChain() = default;
    ~ZcBmResbufChain() = default;

    ZcBmResbufChain(const ZcBmResbufChain&) = delete;
    ZcBmResbufChain& operator=(const ZcBmResbufChain&) = delete;

    void append(resbuf* node) noexcept {
        if (node == nullptr) {
            return;
        }
        if (m_head == nullptr) {
            m_head = node;
        } else {
            m_tail->rbnext = node;
        }

        resbuf* tail = node;
        while (tail->rbnext != nullptr) {
            tail = tail->rbnext;
        }
        m_tail = tail;
    }

    resbuf* detach() noexcept {
        resbuf* head = m_head;
        m_head = nullptr;
        m_tail = nullptr;
        return head;
    }

    void release() noexcept {
        if (m_head != nullptr) {
            acutRelRb(m_head);
        }
        m_head = nullptr;
        m_tail = nullptr;
    }

    bool appendTerminator() noexcept {
        resbuf* node = acutNewRb(RTNONE);
        if (node == nullptr) {
            return false;
        }
        append(node);
        return true;
    }

    resbuf* head() const noexcept { return m_head; }

   private:
    resbuf* m_head = nullptr;
    resbuf* m_tail = nullptr;
};

template <typename T>
struct ZcBmResbufCodec;

template <>
struct ZcBmResbufCodec<std::int16_t> {
    static bool decode(const resbuf* node, short restype, std::int16_t* out_value) {
        if (node == nullptr || out_value == nullptr) {
            return false;
        }
        if (node->restype == restype) {
            *out_value = static_cast<std::int16_t>(node->resval.rint);
            return true;
        }
        return false;
    }

    static bool writeValue(resbuf* node, std::int16_t value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.rint = static_cast<short>(value);
        return true;
    }

    static bool encode(short restype, std::int16_t value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<std::int32_t> {
    static bool decode(const resbuf* node, short restype, std::int32_t* out_value) {
        if (node == nullptr || out_value == nullptr) {
            return false;
        }
        if (node->restype != restype) {
            return false;
        }
        *out_value = static_cast<std::int32_t>(node->resval.rlong);
        return true;
    }

    static bool writeValue(resbuf* node, std::int32_t value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.rlong = value;
        return true;
    }

    static bool encode(short restype, std::int32_t value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<std::int64_t> {
    static bool decode(const resbuf* node, short restype, std::int64_t* out_value) {
        if (node == nullptr || out_value == nullptr) {
            return false;
        }
        if (node->restype == restype) {
            *out_value = static_cast<std::int64_t>(node->resval.mnInt64);
            return true;
        }
        return false;
    }

    static bool writeValue(resbuf* node, std::int64_t value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.mnInt64 = value;
        return true;
    }

    static bool encode(short restype, std::int64_t value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<bool> {
    static bool decode(const resbuf* node, short restype, bool* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        *out_value = (node->resval.rint != 0);
        return true;
    }

    static bool writeValue(resbuf* node, bool value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.rint = static_cast<short>(value ? 1 : 0);
        return true;
    }

    static bool encode(short restype, bool value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<double> {
    static bool decode(const resbuf* node, short restype, double* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        *out_value = node->resval.rreal;
        return true;
    }

    static bool writeValue(resbuf* node, double value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.rreal = value;
        return true;
    }

    static bool encode(short restype, double value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<std::wstring> {
    static bool decode(const resbuf* node, short restype, std::wstring* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        const ZTCHAR* text = node->resval.rstring;
        *out_value = (text == nullptr) ? L"" : std::wstring(text);
        return true;
    }

    static bool writeValue(resbuf* node, const std::wstring& value) {
        if (node == nullptr) {
            return false;
        }
        return zcutNewString(value.c_str(), node->resval.rstring) == Zcad::eOk;
    }

    static bool encode(short restype, const std::wstring& value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<ZcString> {
    static bool decode(const resbuf* node, short restype, ZcString* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        const ZTCHAR* text = node->resval.rstring;
        *out_value = (text == nullptr) ? ZcString() : ZcString(text);
        return true;
    }

    static bool writeValue(resbuf* node, const ZcString& value) {
        if (node == nullptr) {
            return false;
        }
        return zcutNewString(value.kTCharPtr(), node->resval.rstring) == Zcad::eOk;
    }

    static bool encode(short restype, const ZcString& value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<ZTCHAR*> {
    static bool decode(const resbuf* node, short restype, ZTCHAR** out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        if (node->resval.rstring == nullptr) {
            *out_value = nullptr;
            return true;
        }
        return zcutNewString(node->resval.rstring, *out_value) == Zcad::eOk;
    }

    static bool writeValue(resbuf* node, const ZTCHAR* value) {
        if (node == nullptr) {
            return false;
        }
        return zcutNewString(value == nullptr ? _T("") : value, node->resval.rstring) == Zcad::eOk;
    }

    static bool encode(short restype, const ZTCHAR* value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<const ZTCHAR*> : ZcBmResbufCodec<ZTCHAR*> {
    static bool decode(const resbuf* node, short restype, const ZTCHAR** out_value) {
        if (out_value == nullptr) {
            return false;
        }
        ZTCHAR* copied = nullptr;
        if (!ZcBmResbufCodec<ZTCHAR*>::decode(node, restype, &copied)) {
            return false;
        }
        *out_value = copied;
        return true;
    }
};

template <>
struct ZcBmResbufCodec<AcGePoint3d> {
    static bool decode(const resbuf* node, short restype, AcGePoint3d* out_value) {
        if (node == nullptr || out_value == nullptr) {
            return false;
        }
        if (node->restype != restype) {
            return false;
        }
        *out_value = AcGePoint3d(node->resval.rpoint[X], node->resval.rpoint[Y], node->resval.rpoint[Z]);
        return true;
    }

    static bool writeValue(resbuf* node, const AcGePoint3d& value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.rpoint[X] = value.x;
        node->resval.rpoint[Y] = value.y;
        node->resval.rpoint[Z] = value.z;
        return true;
    }

    static bool encode(short restype, const AcGePoint3d& value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<zds_binary> {
    static bool decode(const resbuf* node, short restype, zds_binary* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        out_value->clen = node->resval.rbinary.clen;
        out_value->buf = node->resval.rbinary.buf;
        return true;
    }

    static bool writeValue(resbuf* node, const zds_binary& value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.rbinary.clen = value.clen;
        node->resval.rbinary.buf = value.buf;
        return true;
    }

    static bool encode(short restype, const zds_binary& value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<zds_name> {
    static bool decode(const resbuf* node, short restype, zds_name* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        (*out_value)[0] = node->resval.rlname[0];
        (*out_value)[1] = node->resval.rlname[1];
        return true;
    }

    static bool writeValue(resbuf* node, const zds_name& value) {
        if (node == nullptr) {
            return false;
        }
        node->resval.rlname[0] = value[0];
        node->resval.rlname[1] = value[1];
        return true;
    }

    static bool encode(short restype, const zds_name& value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<AcDbObjectId> {
    static bool decode(const resbuf* node, short restype, AcDbObjectId* out_value) {
        if (node == nullptr || out_value == nullptr || node->restype != restype) {
            return false;
        }
        zds_name name = {0, 0};
        name[0] = node->resval.rlname[0];
        name[1] = node->resval.rlname[1];
        return acdbGetObjectId(*out_value, name) == Zcad::eOk;
    }

    static bool writeValue(resbuf* node, const AcDbObjectId& value) {
        if (node == nullptr || value.isNull()) {
            return false;
        }
        zds_name name = {0, 0};
        if (acdbGetAdsName(name, value) != Zcad::eOk) {
            return false;
        }
        node->resval.rlname[0] = name[0];
        node->resval.rlname[1] = name[1];
        return true;
    }

    static bool encode(short restype, const AcDbObjectId& value, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr || !writeValue(node, value)) {
            if (node != nullptr) {
                acutRelRb(node);
            }
            return false;
        }
        chain->append(node);
        return true;
    }
};

template <>
struct ZcBmResbufCodec<ZcBmNilTag> {
    static bool decode(const resbuf* node, short restype, ZcBmNilTag* out_value) {
        if (out_value == nullptr) {
            return false;
        }
        return node != nullptr && node->restype == restype;
    }

    static bool writeValue(resbuf* /*node*/, ZcBmNilTag /*value*/) { return true; }

    static bool encode(short restype, ZcBmNilTag /*value*/, ZcBmResbufChain* chain) {
        if (chain == nullptr) {
            return false;
        }
        resbuf* node = acutNewRb(restype);
        if (node == nullptr) {
            return false;
        }
        chain->append(node);
        return true;
    }
};
