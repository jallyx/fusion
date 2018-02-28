#pragma once

#include <cstring>
#include <string>
#include <lua.hpp>

namespace lua {

lua_State *open();
void close(lua_State *L);
void init(lua_State *L);

// string-buffer excution
void dofile(lua_State *L, const char *filename);
void dostring(lua_State *L, const char *buff);
void dobuffer(lua_State *L, const char *buff, size_t sz);

// debug helpers
void print_error(lua_State *L, const char *fmt, ...);
int on_error(lua_State *L);
void enum_stack(lua_State *L);

// class property
template<typename T> struct class_property { static char name[]; };
template<typename T> char class_property<T>::name[256] = "";
template<typename T> struct class_type {
    typedef typename std::remove_cv<
        typename std::remove_reference<
            typename std::remove_pointer<T>::type
        >::type
    >::type type;
    static void init(const char *name) {
        std::strcpy(class_property<type>::name, name);
    }
    static const char *name() {
        return class_property<type>::name;
    }
    static bool is_registered() {
        return class_property<type>::name[0] != '\0';
    }
};

// class metatable
inline void push_metatable(lua_State *L, const char *name) {
    luaL_getmetatable(L, name);
}
inline void push_gc_metatable(lua_State *L) {
#define GC_META_NAME "(_GC_META_)"
    luaL_getmetatable(L, GC_META_NAME);
}
inline void bind_metatable(lua_State *L, const char *name) {
    if (lua_type(L, -1) == LUA_TUSERDATA) {
        push_metatable(L, name);
        lua_setmetatable(L, -2);
    }
}
inline void bind_gc_metatable(lua_State *L) {
    if (lua_type(L, -1) == LUA_TUSERDATA) {
        push_gc_metatable(L);
        lua_setmetatable(L, -2);
    }
}
template<typename T>
inline void bind_proper_metatable(lua_State *L) {
    if (class_type<T>::is_registered()) {
        bind_metatable(L, class_type<T>::name());
    } else {
        bind_gc_metatable(L);
    }
}

// userdata holder
struct user {
    user(void *p) : obj(p) {}
    virtual ~user() {}
    void *obj;
};

// trace object handler
class tracer {
protected:
    tracer() = default;
    ~tracer() = default;
};

// bind object handler
class binder : public tracer {
public:
    bool try_push_this(lua_State *L, const char *metaname) const;
    void bind(lua_State *L, const char *name);
    void unbind(void *user, const char *name);
protected:
    binder() : L_(nullptr), user_(nullptr) {}
    binder(const binder &) : binder() {}
    binder &operator=(const binder &) { return *this; }
    ~binder() { on_destroy(); }
private:
    void register_this(lua_State *L) const;
    void on_destroy() const;
    lua_State *L_;
    user *user_;
};

// adapt object behavior
class adapter {
public:
    template<typename T> struct tracker_subclass {
        static void *base(T *obj) {
            return ((void*)static_cast<const tracer *>(obj));
        }
        static T *self(void *obj) {
            return static_cast<T*>(reinterpret_cast<tracer*>(obj));
        }
    };
    template<typename T> struct non_tracker_subclass {
        static void *base(T *obj) { return ((void*)obj); }
        static T *self(void *obj) { return reinterpret_cast<T*>(obj); }
    };
    template<typename T>
    static void *base(T *obj) {
        return std::conditional<std::is_base_of<tracer, T>::value,
                   tracker_subclass<T>, non_tracker_subclass<T>
               >::type::base(obj);
    }
    template<typename T>
    static T *self(void *obj) {
        return std::conditional<std::is_base_of<tracer, T>::value,
                   tracker_subclass<T>, non_tracker_subclass<T>
               >::type::self(obj);
    }

    template<typename T> struct binder_subclass {
        static void bind(lua_State *L, user *puser) {
            self<T>(puser->obj)->bind(L, puser, class_type<T>::name());
        }
        static void unbind(user *puser) {
            if (puser->obj != nullptr) {
                self<T>(puser->obj)->unbind(puser, class_type<T>::name());
            }
        }
    };
    template<typename T> struct non_binder_subclass {
        static void bind(lua_State *L, user *puser) {}
        static void unbind(user *puser) {}
    };
    template<typename T>
    static void bind(lua_State *L, user *puser) {
        return std::conditional<std::is_base_of<binder, T>::value,
                   binder_subclass<T>, non_binder_subclass<T>
               >::type::bind(L, puser);
    }
    template<typename T>
    static void unbind(user *puser) {
        return std::conditional<std::is_base_of<binder, T>::value,
                   binder_subclass<T>, non_binder_subclass<T>
               >::type::unbind(puser);
    }
};

// from lua
template<typename T>
struct void2ptr {
    static T *invoke(void *input) {
        return adapter::self<T>(input);
    }
};
template<typename T>
struct void2ref {
    static T &invoke(void *input) {
        return *adapter::self<T>(input);
    }
};
template<typename T>
struct void2val {
    static T invoke(void *input) {
        return *adapter::self<T>(input);
    }
};

template<typename T>
struct void2type {
    static T invoke(void *ptr) {
        return std::conditional<std::is_pointer<T>::value,
                   void2ptr<typename std::remove_pointer<T>::type>,
                   typename std::conditional<std::is_lvalue_reference<T>::value,
                       void2ref<typename std::remove_reference<T>::type>,
                       void2val<typename std::remove_reference<T>::type>
                   >::type
               >::type::invoke(ptr);
    }
};

template<typename T>
struct lua2object {
    static T invoke(lua_State *L, int index) {
        return void2type<T>::invoke(lua_touserdata(L, index));
    }
};

template<typename T>
struct lua2enum {
    static T invoke(lua_State *L, int index) {
        return static_cast<T>(lua_tointeger(L, index));
    }
};

template<typename T>
struct lua2user {
    static T invoke(lua_State *L, int index) {
        if (!lua_isuserdata(L, index) && !lua_isnoneornil(L, index)) {
            luaL_error(L, "argument types do not match.");
        }
        return void2type<T>::invoke(lua_isnoneornil(L, index) ? nullptr :
                   (lua_islightuserdata(L, index) ? lua_touserdata(L, index) :
                    assert_userdata_alive(L, lua2object<user*>::invoke(L, index))));
    }
    static void *assert_userdata_alive(lua_State *L, user *puser) {
        if (puser->obj == nullptr) {
            luaL_error(L, "userdata[%p] is a nil value.", puser);
        }
        return puser->obj;
    }
};

// to lua
template<typename T>
struct ptr2user : user {
    ptr2user(T *ptr) : user(adapter::base<T>(ptr)) {}
    virtual ~ptr2user() { adapter::unbind<T>(this); }
};
template<typename T>
struct val2user : user {
    template<typename... Args>
    val2user(Args&&... args) : user(adapter::base<T>(new T(std::forward<Args>(args)...))) {}
    virtual ~val2user() { adapter::unbind<T>(this); delete adapter::self<T>(obj); }
};

template<typename T>
struct binder2lua {
    template<typename... Args>
    static void new_user_object(lua_State *L, Args&&... args) {
        user *puser = new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(std::forward<Args>(args)...);
        adapter::self<T>(puser->obj)->bind(L, class_type<T>::name());
        bind_proper_metatable<T>(L);
    }
    static void invoke(lua_State *L, T *input) {
        if (!input->try_push_this(L, class_type<T>::name())) {
            new(lua_newuserdata(L, sizeof(ptr2user<T>))) ptr2user<T>(input);
            input->bind(L, class_type<T>::name());
            bind_proper_metatable<T>(L);
        }
    }
};
template<typename T>
struct nonbinder2lua {
    template<typename... Args>
    static void new_user_object(lua_State *L, Args&&... args) {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(std::forward<Args>(args)...);
        bind_proper_metatable<T>(L);
    }
    static void invoke(lua_State *L, T *input) {
        new(lua_newuserdata(L, sizeof(ptr2user<T>))) ptr2user<T>(input);
        bind_proper_metatable<T>(L);
    }
};

template<typename T>
struct ptr2lua {
    static void invoke(lua_State *L, T *input) {
        if (input != nullptr) {
            if (class_type<T>::is_registered()) {
                std::conditional<std::is_base_of<binder, T>::value,
                    binder2lua<T>, nonbinder2lua<T>
                >::type::invoke(L, input);
            } else {
                lua_pushlightuserdata(L, adapter::base<T>(input));
            }
        } else {
            lua_pushnil(L);
        }
    }
};
template<typename T>
struct ref2lua {
    static void invoke(lua_State *L, T &input) {
        if (class_type<T>::is_registered()) {
            std::conditional<std::is_base_of<binder, T>::value,
                binder2lua<T>, nonbinder2lua<T>
            >::type::invoke(L, &input);
        } else {
            lua_pushlightuserdata(L, adapter::base<T>(&input));
        }
    }
};
template<typename T>
struct val2lua {
    static void invoke(lua_State *L, T &&input) {
        std::conditional<std::is_base_of<binder, T>::value,
            binder2lua<T>, nonbinder2lua<T>
        >::type::new_user_object(L, std::forward<T>(input));
    }
};

template<typename T>
struct enum2lua {
    static void invoke(lua_State *L, T val) {
        lua_pushinteger(L, static_cast<lua_Integer>(val));
    }
};

template<typename T>
struct user2lua {
    static void invoke(lua_State *L, T val) {
        std::conditional<std::is_pointer<T>::value,
            ptr2lua<typename std::remove_pointer<T>::type>,
            typename std::conditional<std::is_lvalue_reference<T>::value,
                ref2lua<typename std::remove_reference<T>::type>,
                val2lua<typename std::remove_reference<T>::type>
            >::type
        >::type::invoke(L, std::forward<T>(val));
    }
};

// string wrapper
struct sstr {
    sstr() : str(nullptr), len(0) {}
    sstr(const char *s, size_t l) : str(s), len(l) {}
    sstr(const std::string &s) : str(s.data()), len(s.size()) {}
    const char *str;
    size_t len;
};

// read a value from lua stack
template<typename T>
struct read {
    static T invoke(lua_State *L, int index) {
        return std::conditional<std::is_enum<T>::value,
                   lua2enum<T>,
                   lua2user<T>
               >::type::invoke(L, index);
    }
};

#define READ_VALUE_FROM_LUA_STACK(TYPE,FUNC) \
    template<> struct read<TYPE> { \
        static TYPE invoke(lua_State *L, int index) { \
            return static_cast<TYPE>(FUNC(L, index)); \
        } \
    }; \
    template<> struct read<const TYPE> { \
        static TYPE invoke(lua_State *L, int index) { \
            return static_cast<TYPE>(FUNC(L, index)); \
        } \
    }; \
    template<> struct read<const TYPE &> { \
        static TYPE invoke(lua_State *L, int index) { \
            return static_cast<TYPE>(FUNC(L, index)); \
        } \
    };
    READ_VALUE_FROM_LUA_STACK(char32_t, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(char16_t, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(wchar_t, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(char, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(signed char, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(unsigned char, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(signed short, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(unsigned short, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(signed int, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(unsigned int, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(signed long, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(unsigned long, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(signed long long, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(unsigned long long, lua_tointeger)
    READ_VALUE_FROM_LUA_STACK(long double, lua_tonumber)
    READ_VALUE_FROM_LUA_STACK(double, lua_tonumber)
    READ_VALUE_FROM_LUA_STACK(float, lua_tonumber)
#undef READ_VALUE_FROM_LUA_STACK

#define READ_VALUE_FROM_LUA_STACK(TYPE,FUNC) \
    template<> struct read<TYPE> { \
        static TYPE invoke(lua_State *L, int index) { \
            return static_cast<TYPE>(FUNC(L, index)); \
        } \
    };
    READ_VALUE_FROM_LUA_STACK(const char *, lua_tostring)
#undef READ_VALUE_FROM_LUA_STACK

#define READ_VALUE_FROM_LUA_STACK(TYPE,INSTRUCTIONS) \
    template<> struct read<TYPE> { \
        static TYPE invoke(lua_State *L, int index) { \
            INSTRUCTIONS; \
        } \
    }; \
    template<> struct read<const TYPE> { \
        static TYPE invoke(lua_State *L, int index) { \
            INSTRUCTIONS; \
        } \
    }; \
    template<> struct read<const TYPE &> { \
        static TYPE invoke(lua_State *L, int index) { \
            INSTRUCTIONS; \
        } \
    };
    READ_VALUE_FROM_LUA_STACK(bool, do{
        return lua_toboolean(L, index) != 0;
    }while(0))
    READ_VALUE_FROM_LUA_STACK(std::string, do{
        size_t len;
        const char *str = lua_tolstring(L, index, &len);
        return std::string(str, len);
    }while(0))
    READ_VALUE_FROM_LUA_STACK(sstr, do{
        sstr s;
        s.str = lua_tolstring(L, index, &s.len);
        return s;
    }while(0))
#undef READ_VALUE_FROM_LUA_STACK

// push a value to lua stack
template<typename T>
struct push {
    static void invoke(lua_State *L, T val) {
        std::conditional<std::is_enum<T>::value,
            enum2lua<T>,
            user2lua<T>
        >::type::invoke(L, std::forward<T>(val));
    }
};

template<> struct push<std::nullptr_t> {
    static void invoke(lua_State *L, std::nullptr_t) {
        lua_pushnil(L);
    }
};

#define PUSH_VALUE_TO_LUA_STACK(CTYPE,LTYPE,FUNC) \
    template<> struct push<CTYPE> { \
        static void invoke(lua_State *L, CTYPE val) { \
            FUNC(L, static_cast<LTYPE>(val)); \
        } \
    }; \
    template<> struct push<const CTYPE> { \
        static void invoke(lua_State *L, CTYPE val) { \
            FUNC(L, static_cast<LTYPE>(val)); \
        } \
    }; \
    template<> struct push<const CTYPE &> { \
        static void invoke(lua_State *L, CTYPE val) { \
            FUNC(L, static_cast<LTYPE>(val)); \
        } \
    };
    PUSH_VALUE_TO_LUA_STACK(char32_t, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(char16_t, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(wchar_t, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(char, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(signed char, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(unsigned char, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(signed short, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(unsigned short, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(signed int, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(unsigned int, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(signed long, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(unsigned long, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(signed long long, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(unsigned long long, lua_Integer, lua_pushinteger)
    PUSH_VALUE_TO_LUA_STACK(long double, lua_Number, lua_pushnumber)
    PUSH_VALUE_TO_LUA_STACK(double, lua_Number, lua_pushnumber)
    PUSH_VALUE_TO_LUA_STACK(float, lua_Number, lua_pushnumber)
    PUSH_VALUE_TO_LUA_STACK(bool, int, lua_pushboolean)
#undef PUSH_VALUE_TO_LUA_STACK

#define PUSH_VALUE_TO_LUA_STACK(CTYPE,LTYPE,FUNC) \
    template<> struct push<CTYPE> { \
        static void invoke(lua_State *L, CTYPE val) { \
            FUNC(L, static_cast<LTYPE>(val)); \
        } \
    };
    PUSH_VALUE_TO_LUA_STACK(char *, const char *, lua_pushstring)
    PUSH_VALUE_TO_LUA_STACK(const char *, const char *, lua_pushstring)
#undef PUSH_VALUE_TO_LUA_STACK

#define PUSH_VALUE_TO_LUA_STACK(TYPE,INSTRUCTIONS) \
    template<> struct push<TYPE> { \
        static void invoke(lua_State *L, const TYPE &val) { \
            INSTRUCTIONS; \
        } \
    }; \
    template<> struct push<const TYPE> { \
        static void invoke(lua_State *L, const TYPE &val) { \
            INSTRUCTIONS; \
        } \
    }; \
    template<> struct push<const TYPE &> { \
        static void invoke(lua_State *L, const TYPE &val) { \
            INSTRUCTIONS; \
        } \
    };
    PUSH_VALUE_TO_LUA_STACK(std::string, do{
        lua_pushlstring(L, val.data(), val.size());
    }while(0))
    PUSH_VALUE_TO_LUA_STACK(sstr, do{
        lua_pushlstring(L, val.str, val.len);
    }while(0))
#undef PUSH_VALUE_TO_LUA_STACK

// pop a value from lua stack
template<typename T>
struct pop {
    static T invoke(lua_State *L) {
        T t = read<T>::invoke(L, -1);
        lua_pop(L, 1);
        return t;
    }
};
template<> struct pop<void> {
    static void invoke(lua_State *L) {
        lua_pop(L, 1);
    }
};

// exchange lua variable
template<typename T>
void set(lua_State *L, const char *name, T object) {
    push<T>::invoke(L, object);
    lua_setglobal(L, name);
}

template<typename T>
T get(lua_State *L, const char *name){
    lua_getglobal(L, name);
    return pop<T>::invoke(L);
}

// variadic template utilities
template<std::size_t... Indexes>
struct IndexTuple {
    typedef IndexTuple<Indexes..., sizeof...(Indexes)> __next;
};
template<std::size_t N>
struct BuildIndexTuple {
    typedef typename BuildIndexTuple<N - 1>::__type::__next __type;
};
template<> struct BuildIndexTuple<0> {
    typedef IndexTuple<> __type;
};

// get value from cclosure
template<typename T>
T upvalue(lua_State *L, int index) {
    return lua2object<T>::invoke(L, lua_upvalueindex(index));
}

// global functor (with return value)
template<typename RVal, typename... Args>
struct functor {
    static int invoke(lua_State *L) {
        typedef typename BuildIndexTuple<sizeof...(Args)>::__type BoundIndexes;
        invoke(L, BoundIndexes());
        return 1;
    }
    template<std::size_t... Indexes>
    static void invoke(lua_State *L, IndexTuple<Indexes...>) {
        push<RVal>::invoke(L, upvalue<RVal(*)(Args...)>(L, 1)(read<Args>::invoke(L, Indexes + 1)...));
    }
};

// global functor (without return value)
template<typename... Args>
struct functor<void, Args...> {
    static int invoke(lua_State *L) {
        typedef typename BuildIndexTuple<sizeof...(Args)>::__type BoundIndexes;
        invoke(L, BoundIndexes());
        return 0;
    }
    template<std::size_t... Indexes>
    static void invoke(lua_State *L, IndexTuple<Indexes...>) {
        upvalue<void(*)(Args...)>(L, 1)(read<Args>::invoke(L, Indexes + 1)...);
    }
};

// global functor (non-managed)
template<typename... Args>
struct functor<int, lua_State*, Args...> {
    static int invoke(lua_State *L) {
        typedef typename BuildIndexTuple<sizeof...(Args)>::__type BoundIndexes;
        return invoke(L, BoundIndexes());
    }
    template<std::size_t... Indexes>
    static int invoke(lua_State *L, IndexTuple<Indexes...>) {
        return upvalue<int(*)(lua_State*, Args...)>(L, 1)(L, read<Args>::invoke(L, Indexes + 1)...);
    }
};

// push global functor
template<typename RVal, typename... Args>
void push_functor(lua_State *L, RVal(*func)(Args...)) {
    lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
    lua_pushcclosure(L, functor<RVal, Args...>::invoke, 1);
}

// define global function
template<typename F>
void def(lua_State *L, const char *name, F func) {
    push_functor(L, func);
    lua_setglobal(L, name);
}

// class member functor (with return value)
template<typename RVal, typename T, typename... Args>
struct mem_functor {
    static int invoke(lua_State *L) {
        typedef typename BuildIndexTuple<sizeof...(Args)>::__type BoundIndexes;
        invoke(L, BoundIndexes());
        return 1;
    }
    template<std::size_t... Indexes>
    static void invoke(lua_State *L, IndexTuple<Indexes...>) {
        push<RVal>::invoke(L, (read<T*>::invoke(L, 1)->*upvalue<RVal(T::*)(Args...)>(L, 1))(read<Args>::invoke(L, Indexes + 2)...));
    }
};

// class member functor (without return value)
template<typename T, typename... Args>
struct mem_functor<void, T, Args...> {
    static int invoke(lua_State *L) {
        typedef typename BuildIndexTuple<sizeof...(Args)>::__type BoundIndexes;
        invoke(L, BoundIndexes());
        return 0;
    }
    template<std::size_t... Indexes>
    static void invoke(lua_State *L, IndexTuple<Indexes...>) {
        (read<T*>::invoke(L, 1)->*upvalue<void(T::*)(Args...)>(L, 1))(read<Args>::invoke(L, Indexes + 2)...);
    }
};

// class member functor (non-managed)
template<typename T, typename... Args>
struct mem_functor<int, T, lua_State*, Args...> {
    static int invoke(lua_State *L) {
        typedef typename BuildIndexTuple<sizeof...(Args)>::__type BoundIndexes;
        return invoke(L, BoundIndexes());
    }
    template<std::size_t... Indexes>
    static int invoke(lua_State *L, IndexTuple<Indexes...>) {
        return (read<T*>::invoke(L, 1)->*upvalue<int(T::*)(lua_State*, Args...)>(L, 1))(L, read<Args>::invoke(L, Indexes + 2)...);
    }
};

// push class member functor
template<typename RVal, typename T, typename... Args>
void push_functor(lua_State *L, RVal(T::*func)(Args...)) {
    new(lua_newuserdata(L, sizeof(func))) decltype(func)(func);
    lua_pushcclosure(L, mem_functor<RVal, T, Args...>::invoke, 1);
}
template<typename RVal, typename T, typename... Args>
void push_functor(lua_State *L, RVal(T::*func)(Args...)const) {
    new(lua_newuserdata(L, sizeof(func))) decltype(func)(func);
    lua_pushcclosure(L, mem_functor<RVal, T, Args...>::invoke, 1);
}

// class constructor
template<typename T, typename... Args, std::size_t... Indexes>
void new_user_object(lua_State *L, IndexTuple<Indexes...>) {
    std::conditional<std::is_base_of<binder, T>::value,
        binder2lua<T>, nonbinder2lua<T>
    >::type::new_user_object(L, read<Args>::invoke(L, Indexes + 2)...);
}
template<typename T, typename... Args>
int constructor(lua_State *L) {
    typedef typename BuildIndexTuple<sizeof...(Args)>::__type BoundIndexes;
    new_user_object<T, Args...>(L, BoundIndexes());
    return 1;
}

// class destroyer
template<typename T>
int destroyer(lua_State *L) {
    lua2object<user*>::invoke(L, -1)->~user();
    return 0;
}

// class member variable
struct var_base {
    virtual void get(lua_State *L) = 0;
    virtual void set(lua_State *L) = 0;
};

template<typename T, typename V>
class mem_var : var_base {
public:
    mem_var(V T::*val) : var_(val) {}
    typedef typename std::conditional<!std::is_scalar<V>::value,
        typename std::add_lvalue_reference<V>::type, V
    >::type type;
    virtual void get(lua_State *L) {
        push<type>::invoke(L, read<T*>::invoke(L, 1)->*var_);
    }
    struct setter {
        static void invoke(lua_State *L, V T::*var) {
            read<T*>::invoke(L, 1)->*var = read<type>::invoke(L, 3);
        }
    };
    struct noop_setter {
        static void invoke(lua_State *L, V T::*var) {
        }
    };
    virtual void set(lua_State *L) {
        std::conditional<std::is_copy_assignable<V>::value,
            setter, noop_setter
        >::type::invoke(L, var_);
    }
private:
    V T::*var_;
};

// class helper
void register_constructor(lua_State *L, const char *name);
void register_prototype(lua_State *L, const char *name);
void register_parent(lua_State *L, const char *name, const char *parent);

// class init
template<typename T>
void class_add(lua_State *L, const char *name) {
    class_type<T>::init(name);
    register_prototype(L, name);
}

// class inheritence
template<typename T, typename P>
void class_inh(lua_State *L) {
    register_parent(L, class_type<T>::name(), class_type<P>::name());
}

// class cast object
template<typename T, typename P>
P *cast(T *obj) { return obj; }
template<typename T, typename P>
void class_cast(lua_State *L) {
    push_metatable(L, class_type<T>::name());
    if (lua_istable(L, -1)) {
        lua_pushfstring(L, "castTo%s", class_type<P>::name());
        push_functor(L, cast<T, P>);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

// class constructor
template<typename T, typename F>
void class_con(lua_State *L, F func) {
    push_metatable(L, class_type<T>::name());
    lua_pushcfunction(L, func);
    register_constructor(L, nullptr);
}
template<typename F>
void class_con(lua_State *L, const char *name, F func) {
    luaL_newmetatable(L, name);
    lua_pushcfunction(L, func);
    register_constructor(L, name);
}

// class functions
template<typename T, typename F>
void class_def(lua_State *L, const char *name, F func) {
    push_metatable(L, class_type<T>::name());
    if (lua_istable(L, -1)) {
        lua_pushstring(L, name);
        push_functor(L, func);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

// class variables
template<typename T, typename BASE, typename VAR>
void class_mem(lua_State *L, const char *name, VAR BASE::*val) {
    push_metatable(L, class_type<T>::name());
    if (lua_istable(L, -1)) {
        lua_pushstring(L, name);
        new(lua_newuserdata(L, sizeof(mem_var<BASE, VAR>))) mem_var<BASE, VAR>(val);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

}
