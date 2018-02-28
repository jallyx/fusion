#pragma once

#include <functional>

class AsyncTaskOwner;

class AsyncTask
{
public:
    AsyncTask() = default;
    virtual ~AsyncTask() = default;

    virtual void Finish(AsyncTaskOwner *owner) {}
    virtual void ExecuteInAsync() = 0;
};

inline AsyncTask *CreateAsyncTask(const std::function<void()> &work)
{
    class Task : public AsyncTask {
    public:
        Task(const std::function<void()> &work)
            : work_(work)
        {}
    private:
        virtual void ExecuteInAsync() {
            work_();
        }
        const std::function<void()> work_;
    };
    return new Task(work);
}

inline AsyncTask *CreateAsyncTask(const std::function<void()> &work,
                                  const std::function<void(AsyncTaskOwner*)> &cb)
{
    class Task : public AsyncTask {
    public:
        Task(const std::function<void()> &work,
             const std::function<void(AsyncTaskOwner*)> &cb)
            : work_(work)
            , cb_(cb)
        {}
    private:
        virtual void Finish(AsyncTaskOwner *owner) {
            if (cb_) { cb_(owner); }
        }
        virtual void ExecuteInAsync() {
            if (work_) { work_(); }
        }
        const std::function<void()> work_;
        const std::function<void(AsyncTaskOwner*)> cb_;
    };
    return new Task(work, cb);
}

template <typename T, typename... Args>
inline AsyncTask *CreateAsyncTask(const std::function<void(T&)> &work,
                                  const std::function<void(AsyncTaskOwner*, T&)> &cb,
                                  Args&&... args)
{
    class Task : public AsyncTask {
    public:
        Task(const std::function<void(T&)> &work,
             const std::function<void(AsyncTaskOwner*, T&)> &cb,
             Args&&... args)
            : work_(work)
            , cb_(cb)
            , msg_(std::forward<Args>(args)...)
        {}
    private:
        virtual void Finish(AsyncTaskOwner *owner) {
            if (cb_) { cb_(owner, msg_); }
        }
        virtual void ExecuteInAsync() {
            if (work_) { work_(msg_); }
        }
        const std::function<void(T&)> work_;
        const std::function<void(AsyncTaskOwner*, T&)> cb_;
        T msg_;
    };
    return new Task(work, cb, std::forward<Args>(args)...);
}
