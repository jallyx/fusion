#pragma once

class noncopyable
{
public:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable(const noncopyable &other) = delete;
    noncopyable &operator=(const noncopyable &other) = delete;
};
