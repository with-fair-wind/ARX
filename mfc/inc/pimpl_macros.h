#pragma once

#include <memory>  // IWYU pragma: keep

// ---- 基础对象语义宏（按项目命名标准 ZCBM_*）----
// 禁用拷贝：优先复用项目已有 DISABLE_COPY。
#ifndef ZCBM_DISABLE_COPY
#ifdef DISABLE_COPY
#define ZCBM_DISABLE_COPY(Class) DISABLE_COPY(Class)
#else
#define ZCBM_DISABLE_COPY(Class)  \
    Class(const Class&) = delete; \
    Class& operator=(const Class&) = delete
#endif
#endif

#ifndef ZCBM_DISABLE_MOVE
#ifdef DISABLE_MOVE
#define ZCBM_DISABLE_MOVE(Class) DISABLE_MOVE(Class)
#else
#define ZCBM_DISABLE_MOVE(Class) \
    Class(Class&&) = delete;     \
    Class& operator=(Class&&) = delete
#endif
#endif

#ifndef ZCBM_DISABLE_COPY_AND_MOVE
#ifdef DISABLE_COPY_AND_MOVE
#define ZCBM_DISABLE_COPY_AND_MOVE(Class) DISABLE_COPY_AND_MOVE(Class)
#else
#define ZCBM_DISABLE_COPY_AND_MOVE(Class) \
    ZCBM_DISABLE_COPY(Class);             \
    ZCBM_DISABLE_MOVE(Class)
#endif
#endif

// 单例（Meyers Singleton）：线程安全局部静态对象（C++11+）。
// 用法：在类定义内部直接写 ZCBM_DECLARE_SINGLETON(YourClass)
#define ZCBM_DECLARE_SINGLETON(Class) \
   public:                            \
    static Class& instance() {        \
        static Class instance;        \
        return instance;              \
    }                                 \
                                      \
   private:                           \
    Class() = default;                \
    ~Class() = default;               \
    ZCBM_DISABLE_COPY_AND_MOVE(Class)

// ---- 标准 PImpl 宏（推荐使用 ZCBM_*）----
#define ZCBM_DECLARE_DPTR(Class)                                        \
    inline Class##Private* d_ptr() { return d_ptr_.get(); }             \
    inline const Class##Private* d_ptr() const { return d_ptr_.get(); } \
    friend class Class##Private;
#define ZCBM_DECLARE_QPTR(Class)                         \
    inline Class* q_ptr() { return q_ptr_; }             \
    inline const Class* q_ptr() const { return q_ptr_; } \
    friend class Class;

#define ZCBM_DPTR(Class) Class##Private* const d = d_ptr()
#define ZCBM_QPTR(Class) Class* const q = q_ptr()

#define ZCBM_PIMPL_OWNER(Class)             \
   protected:                               \
    std::unique_ptr<Class##Private> d_ptr_; \
                                            \
   private:                                 \
    ZCBM_DISABLE_COPY(Class);               \
    ZCBM_DECLARE_DPTR(Class)

#define ZCBM_PIMPL_OBJECT(Class) \
   protected:                    \
    Class* q_ptr_ = nullptr;     \
                                 \
   private:                      \
    ZCBM_DECLARE_QPTR(Class)
