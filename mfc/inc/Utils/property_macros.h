#pragma once

// 类属性 getter/setter 快捷宏（ZCBM_PROPERTY_CREATE 系列）。
//
// 参数说明：
//   Class  — 所属类名（用于 setter 返回 Class& 实现链式调用）
//   Type   — 属性类型
//   Name   — 访问器名（getter 为 get##Name，setter 为 set##Name）
//   Member — 成员变量名（直接使用，不自动加 m_ 前缀）
//
// 用法示例：
//
//   class Wall {
//   public:
//       // 值类型
//       ZCBM_PROPERTY_CREATE(Wall, int, Width, m_width)
//       // 返回 const 引用
//       ZCBM_PROPERTY_CREATE_CREF(Wall, std::wstring, Name, m_name)
//       // 只读
//       ZCBM_PROPERTY_CREATE_READONLY(int, Id, m_id)
//       // 返回 const 引用只读
//       ZCBM_PROPERTY_CREATE_CREF_READONLY(std::wstring, Tag, m_tag)
//       // 布尔属性 is/set
//       ZCBM_PROPERTY_CREATE_BOOL(Wall, Visible, m_visible)
//
//   private:
//       int m_width = 0;
//       std::wstring m_name;
//       int m_id = 0;
//       std::wstring m_tag;
//       bool m_visible = true;
//   };
//
//   // 链式调用：
//   wall.setWidth(200).setName(L"外墙").setVisible(true);

// ==================================================================
// 值类型（按值传参、按值返回，适合 int/double/enum/指针等轻量类型）
// ==================================================================

// 可读写，setter 返回 Class& 支持链式调用。
#define ZCBM_PROPERTY_CREATE(Class, Type, Name, Member) \
    Type get##Name() const { return Member; }           \
    Class& set##Name(const Type& val) {                 \
        Member = val;                                   \
        return *this;                                   \
    }

// 只读（仅 getter，无 setter）。
#define ZCBM_PROPERTY_CREATE_READONLY(Type, Name, Member) \
    Type get##Name() const { return Member; }

// ==================================================================
// 常量引用类型（getter 返回 const Type&，适合 string/vector 等重型对象）
// ==================================================================

// 可读写，getter 返回 const&，setter 接受 const& 并提供移动重载。
#define ZCBM_PROPERTY_CREATE_CREF(Class, Type, Name, Member)   \
    const Type& get##Name() const { return Member; }           \
    Class& set##Name(const Type& val) {                        \
        Member = val;                                          \
        return *this;                                          \
    }                                                          \
    Class& set##Name(Type&& val) {                             \
        Member = std::move(val);                               \
        return *this;                                          \
    }

// 只读，getter 返回 const&。
#define ZCBM_PROPERTY_CREATE_CREF_READONLY(Type, Name, Member) \
    const Type& get##Name() const { return Member; }

// ==================================================================
// 布尔属性（getter 使用 is 前缀）
// ==================================================================

// 可读写布尔属性。
#define ZCBM_PROPERTY_CREATE_BOOL(Class, Name, Member) \
    bool is##Name() const { return Member; }           \
    Class& set##Name(bool val) {                       \
        Member = val;                                  \
        return *this;                                  \
    }

// 只读布尔属性。
#define ZCBM_PROPERTY_CREATE_BOOL_READONLY(Name, Member) \
    bool is##Name() const { return Member; }

// ==================================================================
// 带通知回调的属性（setter 中调用成员函数 on##Name##Changed()）
// 需要在类中自行实现 void on##Name##Changed() 方法。
// ==================================================================

// 值类型 + 变更通知。
#define ZCBM_PROPERTY_CREATE_NOTIFY(Class, Type, Name, Member) \
    Type get##Name() const { return Member; }                  \
    Class& set##Name(const Type& val) {                        \
        if (Member != val) {                                   \
            Member = val;                                      \
            on##Name##Changed();                               \
        }                                                      \
        return *this;                                          \
    }

// 引用类型 + 变更通知。
#define ZCBM_PROPERTY_CREATE_CREF_NOTIFY(Class, Type, Name, Member) \
    const Type& get##Name() const { return Member; }                \
    Class& set##Name(const Type& val) {                             \
        if (Member != val) {                                        \
            Member = val;                                           \
            on##Name##Changed();                                    \
        }                                                           \
        return *this;                                               \
    }                                                               \
    Class& set##Name(Type&& val) {                                  \
        if (Member != val) {                                        \
            Member = std::move(val);                                \
            on##Name##Changed();                                    \
        }                                                           \
        return *this;                                               \
    }
