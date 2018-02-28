#pragma once

#include "noncopyable.h"
#include "LuaBus.h"

class LuaRef : public noncopyable
{
public:
    LuaRef(lua_State *L = nullptr, bool doref = false) : L(L) {
        ref_ = doref ? luaL_ref(L, LUA_REGISTRYINDEX) : LUA_NOREF;
    }

    LuaRef(lua_State *L, int index) : L(L) {
        lua_pushvalue(L, index);
        ref_ = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    LuaRef(lua_State *L, const char *name) : L(L) {
        lua_getglobal(L, name);
        ref_ = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    ~LuaRef() {
        if (isref()) {
            luaL_unref(L, LUA_REGISTRYINDEX, ref_);
        }
    }

    LuaRef(LuaRef &&other) {
        L = other.L, ref_ = other.ref_;
        other.ref_ = LUA_NOREF;
    }

    LuaRef &operator=(LuaRef &&other) {
        this->~LuaRef(), new(this) LuaRef(std::move(other));
        return *this;
    }

    lua_State *getL() const { return L; }

    bool isref() const {
        return ref_ != LUA_NOREF && ref_ != LUA_REFNIL;
    }

    void getref() const {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref_);
    }

    void unref() {
        *this = LuaRef();
    }

    template<typename T>
    void Set(T object) {
        lua::push<T>::invoke(L, object);
        *this = LuaRef(L, true);
    }

    template<typename T>
    T Get() const {
        getref();
        return lua::pop<T>::invoke(L);
    }

    template<typename RVal, typename... Args>
    RVal Call(Args... args) const {
        lua_pushcfunction(L, lua::on_error);
        int errfunc = lua_gettop(L);

        getref();
        if (lua_isfunction(L, -1)) {
            lua::push_args(L, std::forward<Args>(args)...);
            lua_pcall(L, sizeof...(args), 1, errfunc);
        } else {
            lua::print_error(L, "LuaRef::Call() attempt to call '%d' (not a function)", ref_);
        }

        lua_remove(L, errfunc);
        return lua::pop<RVal>::invoke(L);
    }

    template<typename RVal, typename... Args>
    RVal CallMethod(const char *name, Args... args) const {
        lua_pushcfunction(L, lua::on_error);
        int errfunc = lua_gettop(L);

        getref();
        if (lua_istable(L, -1) || lua_type(L, -1) == LUA_TUSERDATA) {
            lua_getfield(L, -1, name);
            if (lua_isfunction(L, -1)) {
                lua_rotate(L, -2, 1);
                lua::push_args(L, std::forward<Args>(args)...);
                lua_pcall(L, sizeof...(args) + 1, 1, errfunc);
            } else {
                lua_remove(L, -2);
                lua::print_error(L, "LuaRef::CallMethod() attempt to call '%s' (not a function)", name);
            }
        } else {
            lua::print_error(L, "LuaRef::CallMethod() attempt to call '%d' (not a table)", ref_);
        }

        lua_remove(L, errfunc);
        return lua::pop<RVal>::invoke(L);
    }

    template<typename RVal, typename... Args>
    RVal CallStaticMethod(const char *name, Args... args) const {
        lua_pushcfunction(L, lua::on_error);
        int errfunc = lua_gettop(L);

        getref();
        if (lua_istable(L, -1) || lua_type(L, -1) == LUA_TUSERDATA) {
            lua_getfield(L, -1, name);
            lua_remove(L, -2);
            if (lua_isfunction(L, -1)) {
                lua::push_args(L, std::forward<Args>(args)...);
                lua_pcall(L, sizeof...(args), 1, errfunc);
            } else {
                lua::print_error(L, "LuaRef::CallStaticMethod() attempt to call '%s' (not a function)", name);
            }
        } else {
            lua::print_error(L, "LuaRef::CallStaticMethod() attempt to call '%d' (not a table)", ref_);
        }

        lua_remove(L, errfunc);
        return lua::pop<RVal>::invoke(L);
    }

private:
    lua_State *L;
    int ref_;
};

namespace lua {

template<> struct pop<LuaRef> {
    static LuaRef invoke(lua_State *L) {
        return LuaRef(L, true);
    }
};
template<> struct pop<const LuaRef> {
    static LuaRef invoke(lua_State *L) {
        return LuaRef(L, true);
    }
};
template<> struct pop<const LuaRef &> {
    static LuaRef invoke(lua_State *L) {
        return LuaRef(L, true);
    }
};
template<> struct pop<LuaRef &&> {
    static LuaRef invoke(lua_State *L) {
        return LuaRef(L, true);
    }
};
template<> struct pop<const LuaRef &&> {
    static LuaRef invoke(lua_State *L) {
        return LuaRef(L, true);
    }
};

template<> struct read<LuaRef> {
    static LuaRef invoke(lua_State *L, int index) {
        return LuaRef(L, index);
    }
};
template<> struct read<const LuaRef> {
    static LuaRef invoke(lua_State *L, int index) {
        return LuaRef(L, index);
    }
};
template<> struct read<const LuaRef &> {
    static LuaRef invoke(lua_State *L, int index) {
        return LuaRef(L, index);
    }
};
template<> struct read<LuaRef &&> {
    static LuaRef invoke(lua_State *L, int index) {
        return LuaRef(L, index);
    }
};
template<> struct read<const LuaRef &&> {
    static LuaRef invoke(lua_State *L, int index) {
        return LuaRef(L, index);
    }
};

template<> struct push<LuaRef> {
    static void invoke(lua_State *L, const LuaRef &val) {
        val.isref() ? val.getref() : lua_pushnil(L);
    }
};
template<> struct push<const LuaRef> {
    static void invoke(lua_State *L, const LuaRef &val) {
        val.isref() ? val.getref() : lua_pushnil(L);
    }
};
template<> struct push<LuaRef &> {
    static void invoke(lua_State *L, const LuaRef &val) {
        val.isref() ? val.getref() : lua_pushnil(L);
    }
};
template<> struct push<const LuaRef &> {
    static void invoke(lua_State *L, const LuaRef &val) {
        val.isref() ? val.getref() : lua_pushnil(L);
    }
};
template<> struct push<LuaRef &&> {
    static void invoke(lua_State *L, const LuaRef &val) {
        val.isref() ? val.getref() : lua_pushnil(L);
    }
};
template<> struct push<const LuaRef &&> {
    static void invoke(lua_State *L, const LuaRef &val) {
        val.isref() ? val.getref() : lua_pushnil(L);
    }
};

}
