#include <algorithm>
#include <cstddef>
#include <memory>
#include <tuple>

template <typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
public:
    UniquePtr() noexcept : ptr_(nullptr), del_(Deleter()) {
    }

    template <class Y>
    UniquePtr(Y *elem) noexcept : ptr_(elem), del_(Deleter()) {
    }

    template <class Y>
    UniquePtr(Y *elem, Deleter d) noexcept : ptr_(elem), del_(d) {
    }

    template <class Y>
    UniquePtr(UniquePtr<Y> &&u_ptr) noexcept {
        if (u_ptr.ptr_ == ptr_) {
            return;
        }
        ptr_ = u_ptr.ptr_;
        del_ = u_ptr.del_;
        u_ptr.ptr_ = nullptr;
    }

    Deleter &GetDeleter() noexcept {
        return del_;
    }

    const Deleter &GetDeleter() const noexcept {
        return del_;
    }

    UniquePtr &operator=(std::nullptr_t) noexcept {
        del_(ptr_);
        ptr_ = nullptr;
        return *this;
    }
    template <class Y>
    UniquePtr &operator=(UniquePtr<Y> &&u_ptr) noexcept {
        if (u_ptr.ptr_ == ptr_) {
            return *this;
        }
        del_(ptr_);
        ptr_ = u_ptr.ptr_;
        del_ = u_ptr.del_;
        u_ptr.ptr_ = nullptr;
        return *this;
    }

    ~UniquePtr() {
        del_(ptr_);
    }

    const T &operator*() const {
        return *ptr_;
    }

    T *operator->() const noexcept {
        return ptr_;
    }

    T *Release() noexcept {
        T *temp_ptr = ptr_;
        ptr_ = nullptr;
        return temp_ptr;
    }

    void Reset(T *p = nullptr) noexcept {
        std::swap(ptr_, p);
        if (p != nullptr) {
            del_(p);
        }
    }

    void Swap(UniquePtr &other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(del_, other.del_);
    }

    T *Get() const noexcept {
        return ptr_;
    }

    explicit operator bool() const noexcept {
        return !(ptr_ == nullptr);
    }

    UniquePtr(UniquePtr &) = delete;

    UniquePtr &operator=(UniquePtr &) = delete;

    T *ptr_;
    Deleter del_;
};
