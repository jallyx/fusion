#pragma once

#include <memory>
#include "InlineFuncs.h"

template <class T>
class enable_linked_from_this
{
public:
    enable_linked_from_this() {
        this_ptr_ = std::shared_ptr<T>(static_cast<T*>(this), noop_deleter);
    }
    enable_linked_from_this(const enable_linked_from_this<T> &obj)
        : enable_linked_from_this()
    {}
    enable_linked_from_this<T> &operator=(const enable_linked_from_this<T> &obj) {
        return *this;
    }
    std::weak_ptr<T> linked_from_this() {
        return this_ptr_;
    }
    std::weak_ptr<const T> linked_from_this() const {
        return this_ptr_;
    }
    void disable_linked_from_this() {
        this_ptr_.reset(nullptr);
    }
private:
    std::shared_ptr<T> this_ptr_;
};
