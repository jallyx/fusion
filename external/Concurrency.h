#pragma once

#if defined(_WIN32)
    #include <concrt.h>
    class spinlock {
    public:
        void lock() { object_.lock(); }
        bool try_lock() { return object_.try_lock(); }
        void unlock() { object_.unlock(); }
    private:
        concurrency::critical_section object_;
    };
    class rwlock {
    public:
        void rdlock() { object_.lock_read(); }
        bool try_rdlock() { return object_.try_lock_read(); }
        void wrlock() { object_.lock(); }
        bool try_wrlock() { return object_.try_lock(); }
        void unlock() { object_.unlock(); }
    private:
        concurrency::reader_writer_lock object_;
    };
#else
    #include <pthread.h>
    class spinlock {
    public:
        spinlock() { pthread_spin_init(&object_, 0); }
        ~spinlock() { pthread_spin_destroy(&object_); }
        void lock() { pthread_spin_lock(&object_); }
        bool try_lock() { return pthread_spin_trylock(&object_) == 0; }
        void unlock() { pthread_spin_unlock(&object_); }
    private:
        pthread_spinlock_t object_;
    };
    class rwlock {
    public:
        rwlock() { pthread_rwlock_init(&object_, nullptr); }
        ~rwlock() { pthread_rwlock_destroy(&object_); }
        void rdlock() { pthread_rwlock_rdlock(&object_); }
        bool try_rdlock() { return pthread_rwlock_tryrdlock(&object_) == 0; }
        void wrlock() { pthread_rwlock_wrlock(&object_); }
        bool try_wrlock() { return pthread_rwlock_trywrlock(&object_) == 0; }
        void unlock() { pthread_rwlock_unlock(&object_); }
    private:
        pthread_rwlock_t object_;
    };
#endif
