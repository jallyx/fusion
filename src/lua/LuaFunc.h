#pragma once

#include "LuaRef.h"
#include "LuaTable.h"

using swallow = int[];

template<typename... Args>
void lua_pushargs(lua_State *L, Args&&... args) {
    swallow{ 0, (lua::push<Args>::invoke(L, std::forward<Args>(args)), 0)... };
}

class LuaFunc : public noncopyable
{
public:
    LuaFunc(lua_State *L, const char *name, bool iswipe = false) : L(L) {
        lua_getglobal(L, name);
        if (init(-1, true)) {
            if (iswipe) {
                lua::set(L, name, nullptr);
            }
        } else {
            lua::print_error(L, "lua attempt to call global '%s' (not a function)", name);
        }
    }

    LuaFunc(lua_State *L, int index, bool isget) : L(L) {
        if (!init(index, isget)) {
            lua::print_error(L, "lua attempt to call stack '%d' (not a function)", index);
        }
    }

    LuaFunc(const LuaRef &t) : L(t.getL()) {
        t.getref();
        if (!init(-1, true)) {
            lua::print_error(L, "lua attempt to call ref '%d' (not a function)", t.index());
        }
    }

    LuaFunc(const LuaTable &t) : L(t.getL()) {
        if (!init(t.index(), false)) {
            lua::print_error(L, "lua attempt to call table '%d' (not a function)", t.index());
        }
    }

    ~LuaFunc() {
        if (errfunc_ != 0) {
            lua_remove(L, errfunc_);
        }
        if (isget_) {
            lua_remove(L, index_);
        }
    }

    LuaFunc(LuaFunc &&other) {
        L = other.L, index_ = other.index_;
        errfunc_ = other.errfunc_, isget_ = other.isget_;
        other.errfunc_ = 0, other.isget_ = false;
    }

    LuaFunc &operator=(LuaFunc &&other) {
        this->~LuaFunc(), new(this) LuaFunc(std::move(other));
        return *this;
    }

    lua_State *getL() const { return L; }
    int index() const { return index_; }

    bool is_alive() const { return errfunc_ != 0; }

    template<typename RVal, typename... Args>
    RVal Call(Args... args) const {
        if (is_alive()) {
            lua_pushvalue(L, index_);
            lua_pushargs(L, std::forward<Args>(args)...);
            lua_pcall(L, sizeof...(args), 1, errfunc_);
        } else {
            lua_pushnil(L);
        }
        return lua::pop<RVal>::invoke(L);
    }

    template<typename... Args>
    bool XCall(int nresults, Args... args) const {
        if (is_alive()) {
            lua_pushvalue(L, index_);
            lua_pushargs(L, std::forward<Args>(args)...);
            lua_pcall(L, sizeof...(args), nresults, errfunc_);
            return true;
        } else {
            return false;
        }
    }

private:
    bool init(int index, bool isget) {
        isget_ = isget;
        index_ = lua_absindex(L, index);
        if (lua_isfunction(L, index_)) {
            lua_pushcfunction(L, lua::on_error);
            errfunc_ = lua_gettop(L);
            return true;
        } else {
            errfunc_ = 0;
            return false;
        }
    }

    lua_State *L;
    int index_;
    int errfunc_;
    bool isget_;
};

class LuaFuncs : public noncopyable
{
public:
    LuaFuncs(lua_State *L, const char *name, bool iswipe = false) : L(L) {
        lua_getglobal(L, name);
        if (init(-1, true)) {
            if (iswipe) {
                lua::set(L, name, nullptr);
            }
        } else {
            lua::print_error(L, "lua attempt to call global '%s' (not a table)", name);
        }
    }

    LuaFuncs(lua_State *L, int index, bool isget) : L(L) {
        if (!init(index, isget)) {
            lua::print_error(L, "lua attempt to call stack '%d' (not a table)", index);
        }
    }

    LuaFuncs(const LuaRef &t) : L(t.getL()) {
        t.getref();
        if (!init(-1, true)) {
            lua::print_error(L, "lua attempt to call ref '%d' (not a table)", t.index());
        }
    }

    LuaFuncs(const LuaTable &t) : L(t.getL()) {
        if (!init(t.index(), false)) {
            lua::print_error(L, "lua attempt to call table '%d' (not a table)", t.index());
        }
    }

    ~LuaFuncs() {
        if (errfunc_ != 0) {
            lua_remove(L, errfunc_);
        }
        if (isget_) {
            lua_remove(L, index_);
        }
    }

    LuaFuncs(LuaFuncs &&other) {
        L = other.L, index_ = other.index_;
        errfunc_ = other.errfunc_, isget_ = other.isget_;
        other.errfunc_ = 0, other.isget_ = false;
    }

    LuaFuncs &operator=(LuaFuncs &&other) {
        this->~LuaFuncs(), new(this) LuaFuncs(std::move(other));
        return *this;
    }

    lua_State *getL() const { return L; }
    int index() const { return index_; }

    bool is_alive() const { return errfunc_ != 0; }

    template<typename RVal, typename... Args>
    RVal CallMethod(const char *name, Args... args) const {
        if (is_alive()) {
            lua_getfield(L, index_, name);
            if (lua_isfunction(L, -1)) {
                lua_pushvalue(L, index_);
                lua_pushargs(L, std::forward<Args>(args)...);
                lua_pcall(L, sizeof...(args)+1, 1, errfunc_);
            } else {
                lua::print_error(L, "lua attempt to call method '%s' (not a function)", name);
            }
        } else {
            lua_pushnil(L);
        }
        return lua::pop<RVal>::invoke(L);
    }

    template<typename RVal, typename... Args>
    RVal CallStaticMethod(const char *name, Args... args) const {
        if (is_alive()) {
            lua_getfield(L, index_, name);
            if (lua_isfunction(L, -1)) {
                lua_pushargs(L, std::forward<Args>(args)...);
                lua_pcall(L, sizeof...(args), 1, errfunc_);
            } else {
                lua::print_error(L, "lua attempt to call static method '%s' (not a function)", name);
            }
        } else {
            lua_pushnil(L);
        }
        return lua::pop<RVal>::invoke(L);
    }

private:
    bool init(int index, bool isget) {
        isget_ = isget;
        index_ = lua_absindex(L, index);
        if (lua_istable(L, index_) || lua_type(L, index_) == LUA_TUSERDATA) {
            lua_pushcfunction(L, lua::on_error);
            errfunc_ = lua_gettop(L);
            return true;
        } else {
            errfunc_ = 0;
            return false;
        }
    }

    lua_State *L;
    int index_;
    int errfunc_;
    bool isget_;
};

namespace lua {

template<> struct pop<LuaFunc> {
    static LuaFunc invoke(lua_State *L) {
        return LuaFunc(L, -1, true);
    }
};
template<> struct pop<const LuaFunc> {
    static LuaFunc invoke(lua_State *L) {
        return LuaFunc(L, -1, true);
    }
};
template<> struct pop<const LuaFunc &> {
    static LuaFunc invoke(lua_State *L) {
        return LuaFunc(L, -1, true);
    }
};
template<> struct pop<LuaFunc &&> {
    static LuaFunc invoke(lua_State *L) {
        return LuaFunc(L, -1, true);
    }
};
template<> struct pop<const LuaFunc &&> {
    static LuaFunc invoke(lua_State *L) {
        return LuaFunc(L, -1, true);
    }
};

template<> struct read<LuaFunc> {
    static LuaFunc invoke(lua_State *L, int index) {
        return LuaFunc(L, index, false);
    }
};
template<> struct read<const LuaFunc> {
    static LuaFunc invoke(lua_State *L, int index) {
        return LuaFunc(L, index, false);
    }
};
template<> struct read<const LuaFunc &> {
    static LuaFunc invoke(lua_State *L, int index) {
        return LuaFunc(L, index, false);
    }
};
template<> struct read<LuaFunc &&> {
    static LuaFunc invoke(lua_State *L, int index) {
        return LuaFunc(L, index, false);
    }
};
template<> struct read<const LuaFunc &&> {
    static LuaFunc invoke(lua_State *L, int index) {
        return LuaFunc(L, index, false);
    }
};

template<> struct push<LuaFunc> {
    static void invoke(lua_State *L, const LuaFunc &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaFunc> {
    static void invoke(lua_State *L, const LuaFunc &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<LuaFunc &> {
    static void invoke(lua_State *L, const LuaFunc &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaFunc &> {
    static void invoke(lua_State *L, const LuaFunc &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<LuaFunc &&> {
    static void invoke(lua_State *L, const LuaFunc &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaFunc &&> {
    static void invoke(lua_State *L, const LuaFunc &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};

}

namespace lua {

template<> struct pop<LuaFuncs> {
    static LuaFuncs invoke(lua_State *L) {
        return LuaFuncs(L, -1, true);
    }
};
template<> struct pop<const LuaFuncs> {
    static LuaFuncs invoke(lua_State *L) {
        return LuaFuncs(L, -1, true);
    }
};
template<> struct pop<const LuaFuncs &> {
    static LuaFuncs invoke(lua_State *L) {
        return LuaFuncs(L, -1, true);
    }
};
template<> struct pop<LuaFuncs &&> {
    static LuaFuncs invoke(lua_State *L) {
        return LuaFuncs(L, -1, true);
    }
};
template<> struct pop<const LuaFuncs &&> {
    static LuaFuncs invoke(lua_State *L) {
        return LuaFuncs(L, -1, true);
    }
};

template<> struct read<LuaFuncs> {
    static LuaFuncs invoke(lua_State *L, int index) {
        return LuaFuncs(L, index, false);
    }
};
template<> struct read<const LuaFuncs> {
    static LuaFuncs invoke(lua_State *L, int index) {
        return LuaFuncs(L, index, false);
    }
};
template<> struct read<const LuaFuncs &> {
    static LuaFuncs invoke(lua_State *L, int index) {
        return LuaFuncs(L, index, false);
    }
};
template<> struct read<LuaFuncs &&> {
    static LuaFuncs invoke(lua_State *L, int index) {
        return LuaFuncs(L, index, false);
    }
};
template<> struct read<const LuaFuncs &&> {
    static LuaFuncs invoke(lua_State *L, int index) {
        return LuaFuncs(L, index, false);
    }
};

template<> struct push<LuaFuncs> {
    static void invoke(lua_State *L, const LuaFuncs &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaFuncs> {
    static void invoke(lua_State *L, const LuaFuncs &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<LuaFuncs &> {
    static void invoke(lua_State *L, const LuaFuncs &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaFuncs &> {
    static void invoke(lua_State *L, const LuaFuncs &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<LuaFuncs &&> {
    static void invoke(lua_State *L, const LuaFuncs &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};
template<> struct push<const LuaFuncs &&> {
    static void invoke(lua_State *L, const LuaFuncs &val) {
        val.is_alive() ? lua_pushvalue(L, val.index()) : lua_pushnil(L);
    }
};

}
