#pragma once

#include <assert.h>
#include "noncopyable.h"

template <class T>
class Singleton : public noncopyable
{
public:
    static T *newInstance() {
        return new T();
    }
    static void deleteInstance() {
        delete instance_;
    }

    static T *instance() {
        return instance_;
    }

protected:
    Singleton() {
        assert(!instance_ && "the singleton object repeat construct.");
        instance_ = static_cast<T*>(this);
    }
    virtual ~Singleton() {
        instance_ = nullptr;
    }

private:
    static T *instance_;
};

template <class T>
T *Singleton<T>::instance_ = nullptr;
