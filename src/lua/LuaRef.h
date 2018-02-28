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
    int index() const { return ref_; }

    bool isref() const {
        return ref_ != LUA_NOREF && ref_ != LUA_REFNIL;
    }

    void getref() const {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref_);
    }

    void unref() {
        *this = LuaRef();
    }

    LuaRef Clone() const {
        getref();
        return LuaRef(L, true);
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
