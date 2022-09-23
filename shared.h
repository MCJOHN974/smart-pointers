#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <cstddef>  // std::nullptr_t
#include <memory>   // std::destroy_at

struct ControlBlockBase {
    int strong = 0;
    int weak = 0;

    ControlBlockBase(int s, int w) : strong(s), weak(w) { }

    void IncreaseStrong() {
        ++strong;
    }

    int UseCount() const {
        return strong;
    }

    void DecreaseStrong() {
        --strong;
        if (!strong) {
            OnZeroStrong();
            OnZeroWeak();
        }
    }

    virtual void OnZeroWeak() = 0;
    virtual void OnZeroStrong() = 0;
    virtual ~ControlBlockBase() = default;
};


template <class T> 
struct ControlBlockMakeShared : public ControlBlockBase {
    //alignas(T) char holder[sizeof(T)];
    std::aligned_storage_t<sizeof(T), alignof(T)> holder;

    template<class... Args>
    ControlBlockMakeShared(Args&&... args) : ControlBlockBase(1, 1) {
        // reinterpret_cast<T*>(&holder)->T::T(args...);
        ::new(&holder) T(std::forward<Args>(args)...);
    }

    void OnZeroStrong() override {
        std::destroy_at(std::launder(reinterpret_cast<T*>(&holder)));
    }

    void OnZeroWeak() override {
        delete this;
    }

    ~ControlBlockMakeShared() override = default;
};


template <class T>
struct ControlBlockPtr : public ControlBlockBase {
    T* ptr;

    ControlBlockPtr() : ControlBlockBase(1, 1) {
        ptr = nullptr;
    }

    ControlBlockPtr(T* p) : ControlBlockBase(1, 1) {
        ptr = p;
    }

    void OnZeroStrong() override {
        delete ptr;
    }
    void OnZeroWeak() override {
        delete this;
    }

    ~ControlBlockPtr() override = default;
};





// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
private:
    ControlBlockBase* cb_;
    T* ptr_;
 
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    template <class A>
    friend class SharedPtr;

    SharedPtr() {
        cb_ = new ControlBlockPtr<T>();
        ptr_ = nullptr;
    }

    SharedPtr(std::nullptr_t) { 
        cb_ = nullptr;
        ptr_ = nullptr;
    }
    
    
    template <class X>
    explicit SharedPtr(X* ptr) {
        cb_ = new ControlBlockPtr<T>(ptr);
        ptr_ = ptr;
    }

    template <class X>
    SharedPtr(const SharedPtr<X>& other) {
        cb_ = other.cb_;
        cb_->IncreaseStrong();
        ptr_ = other.ptr_;
    }

    template <class X>
    SharedPtr(SharedPtr<X>&& other) {
        cb_ = other.cb_;
        other.cb_ = nullptr;
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
    }


    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        cb_ = other.cb_;
        cb_->IncreaseStrong();
        ptr_ = ptr;
    }


    
/*
    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s
*/
    template <class X>
    SharedPtr& operator=(const SharedPtr<X>& other) {
        if (other == *this) {
            return *this;
        }
        cb_->DecreaseStrong();
        cb_ = other.cb_;
        cb_->IncreaseStrong();
        ptr_ = other.ptr_;
        return *this;
    }
    template <class X>    
    SharedPtr& operator=(SharedPtr<X>&& other) {
        if (other == * this) {
            return *this;
        }
        cb_->DecreaseStrong();
        cb_ = other.cb_;
        cb_->IncreaseStrong();
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
        other.cb_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        cb_->DecreaseStrong();
        if (ptr_) {
            delete ptr_;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (cb_) {
            cb_->DecreaseStrong();
        }
        cb_ = nullptr;
        if (ptr_) {
            delete ptr_;
        }
        ptr_ = nullptr;
    }
    void Reset(T* ptr) {
        if (cb_) {
            cb_->DecreaseStrong();
        }
        cb_ = new ControlBlockPtr(ptr);
        if (ptr_) {
            delete ptr_;
        }
        ptr_ = ptr;
    }
    template <class X>
    void Swap(SharedPtr<X>& other) {
        std::swap(cb_, other.cb_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *Get();
    }
    T* operator->() const {
        return Get();
    }
    size_t UseCount() const {
        return cb_->UseCount();
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }    

    template <typename V, typename U>
    friend inline bool operator==(const SharedPtr<V>& left, const SharedPtr<U>& right);
    template <class V, class... Args>
    friend SharedPtr<V> MakeShared(Args&&... args);

    SharedPtr(ControlBlockBase* cb, T* ptr) {
        cb_ = cb;
        ptr_ = ptr;
    }
};

template <class V, class... Args>
SharedPtr<V> MakeShared(Args&&... args) {
    ControlBlockBase* cb = new ControlBlockMakeShared<V>(std::forward<Args>(args)...);
    V* ptr = reinterpret_cast<V*>(&dynamic_cast<ControlBlockMakeShared<V>*>(cb)->holder);
    return SharedPtr<V>(cb, ptr);
}
 
template <typename V, typename U>
inline bool operator==(const SharedPtr<V>& left, const SharedPtr<U>& right) {
    return (void*)left.ptr_ == (void*)right.ptr_;
}

/*
// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};

*/
// std::make_shared<int>(42);