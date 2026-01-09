#pragma once

#ifdef KT_EXPORTARX
#define KT_EXPORTARX_API __declspec(dllexport)
#else
#define KT_EXPORTARX_API __declspec(dllimport)
#endif

#ifndef DEL
#define DEL(ptr)         \
    if (ptr) {           \
        delete (ptr);    \
        (ptr) = nullptr; \
    }
#endif

#define PROPERTY_CREATE_ARX(class, type, name, member) \
    type get##name() const {                           \
        assertReadEnabled();                           \
        return member;                                 \
    }                                                  \
    class& set##name(const type& val) {                \
        assertWriteEnabled();                          \
        member = val;                                  \
        return *this;                                  \
    }
