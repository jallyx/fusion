#pragma once

#include "noncopyable.h"
#include "LuaBus.h"

class LuaTable : public noncopyable
{
public:
    LuaTable(lua_State *L, int index, bool isget) : L(L), isget_(isget) {
        index_ = lua_absindex(L, index);
        pointer_ = lua_istable(L, index) ? lua_topointer(L, index) : nullptr;
    }

    LuaTable(lua_State *L) {
        lua_newtable(L);
        new(this) LuaTable(L, -1, true);
    }

    LuaTable(lua_State *L, const char *name) {
        lua_getglobal(L, name);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setglobal(L, name);
        }
        new(this) LuaTable(L, -1, true);
    }

    ~LuaTable() {
        if (isget_ && is_alive()) {
            lua_remove(L, index_);
        }
    }

    LuaTable(LuaTable &&other) {
        L = other.L, index_ = other.index_;
        isget_ = other.isget_, pointer_ = other.pointer_;
        other.pointer_ = nullptr;
    }

    LuaTable &operator=(LuaTable &&other) {
        this->~LuaTable(), new(this) LuaTable(std::move(other));
        return *this;
    }

    lua_State *getL() const { return L; }
    int index() const { return index_; }

    lua_Integer len() const {
        return is_alive() ? luaL_len(L, index_) : 0;
    }
    bool is_alive() const {
        return pointer_ != nullptr &&
               pointer_ == lua_topointer(L, index_) &&
               lua_istable(L, index_);
    }

    template<typename K, typename T> void set(K key, T object) const {
        if (is_alive()) {
            lua::push<K>::invoke(L, std::forward<K>(key));
            lua::push<T>::invoke(L, std::forward<T>(object));
            lua_settable(L, index_);
        }
    }
    template<typename T, typename K> T get(K key) const {
        if (is_alive()) {
            lua::push<K>::invoke(L, std::forward<K>(key));
            lua_gettable(L, index_);
        } else {
            lua_pushnil(L);
        }
        return lua::pop<T>::invoke(L);
    }

    template<typename T> void set(lua_Integer key, T object) const {
        if (is_alive()) {
            lua::push<T>::invoke(L, std::forward<T>(object));
            lua_seti(L, index_, key);
        }
    }
    template<typename T> T get(lua_Integer key) const {
        if (is_alive()) {
            lua_geti(L, index_, key);
        } else {
            lua_pushnil(L);
        }
        return lua::pop<T>::invoke(L);
    }

    template<typename T> void set(const char *key, T object) const {
        if (is_alive()) {
            lua::push<T>::invoke(L, std::forward<T>(object));
            lua_setfield(L, index_, key);
        }
    }
    template<typename T> T get(const char *key) const {
        if (is_alive()) {
            lua_getfield(L, index_, key);
        } else {
            lua_pushnil(L);
        }
        return lua::pop<T>::invoke(L);
    }

    template<typename... Args> void reset(lua_State *L, Args... args) {
        this->~LuaTable(), new(this) LuaTable(L, std::forward<Args>(args)...);
    }

private:
    lua_State *L;
    int index_;
    bool isget_;
    const void *pointer_;
};

namespace lua {

template<> struct pop<LuaTable> {
    static LuaTable invoke(lua_State *L) {
        return LuaTable(L, -1, true);
    }
};
template<> struct pop<const LuaTable> {
    static LuaTable invoke(lua_State *L) {
        return LuaTable(L, -1, true);
    }
};
template<> struct pop<const LuaTable &> {
    static LuaTable invoke(lua_State *L) {
        return LuaTable(L, -1, true);
    }
};
template<> struct pop<LuaTable &&> {
    static LuaTable invoke(lua_State *L) {
        return LuaTable(L, -1, true);
    }
};
template<> struct pop<const LuaTable &&> {
    static LuaTable invoke(lua_State *L) {
        return LuaTable(L, -1, true);
    }
};

template<> struct read<LuaTable> {
    static LuaTable invoke(lua_State *L, int index) {
        return LuaTable(L, index, false);
    }
};
template<> struct read<const LuaTable> {
    static LuaTable invoke(lua_State *L, int index) {
        return LuaTable(L, index, false);
    }
};
template<> struct read<const LuaTable &> {
    static LuaTable invoke(lua_State *L, int index) {
        return LuaTable(L, index, false);
    }
};
template<> struct read<LuaTable &&> {
    static LuaTable invoke(lua_State *L, int index) {
        return LuaTable(L, index, false);
    }
};
template<> struct read<const LuaTable &&> {
    static LuaTable invoke(lua_State *L, int index) {
        return LuaTable(L, index, false);
    }
};

template<> struct push<LuaTable> {
    static void invoke(lua_State *L, const LuaTable &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaTable> {
    static void invoke(lua_State *L, const LuaTable &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<LuaTable &> {
    static void invoke(lua_State *L, const LuaTable &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaTable &> {
    static void invoke(lua_State *L, const LuaTable &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<LuaTable &&> {
    static void invoke(lua_State *L, const LuaTable &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaTable &&> {
    static void invoke(lua_State *L, const LuaTable &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};

}
