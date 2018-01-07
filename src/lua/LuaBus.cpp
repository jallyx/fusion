#include "LuaBus.h"
#include <cassert>

namespace lua {

#define LITERAL_PARENT "(_PARENT_)"
#define LITERAL_UNIQUE "(_UNIQUE_)"

static void register_gc_metatable(lua_State *L)
{
    luaL_newmetatable(L, GC_META_NAME);

    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, destroyer<void>);
    lua_rawset(L, -3);

    lua_pop(L, 1);
}

static void register_binder_table(lua_State *L)
{
    luaL_newmetatable(L, LITERAL_UNIQUE);

    lua_newtable(L);
    lua_pushliteral(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);

    lua_pop(L, 1);
}

lua_State *open()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    init(L);
    return L;
}

void close(lua_State *L)
{
    lua_close(L);
}

void init(lua_State *L)
{
    register_gc_metatable(L);
    register_binder_table(L);
}

void dofile(lua_State *L, const char *filename)
{
    lua_pushcfunction(L, on_error);
    int errfunc = lua_gettop(L);

    if (luaL_loadfile(L, filename) == 0) {
        lua_pcall(L, 0, 1, errfunc);
    } else {
        print_error(L, "%s", lua_tostring(L, -1));
    }

    lua_pop(L, 1);
    lua_remove(L, errfunc);
}

void dostring(lua_State *L, const char *buff)
{
    dobuffer(L, buff, std::strlen(buff));
}

void dobuffer(lua_State *L, const char *buff, size_t sz)
{
    lua_pushcfunction(L, on_error);
    int errfunc = lua_gettop(L);

    if (luaL_loadbuffer(L, buff, sz, "dobuffer()") == 0) {
        lua_pcall(L, 0, 1, errfunc);
    } else {
        print_error(L, "%s", lua_tostring(L, -1));
    }

    lua_pop(L, 1);
    lua_remove(L, errfunc);
}

static void print_call_stack(lua_State *L)
{
    lua_Debug ar;
    for (int n = 0; lua_getstack(L, n, &ar) == 1; ++n) {
        lua_getinfo(L, "nSltu", &ar);

        const char *indent = "\t";
        if (n == 0) {
            indent = "->\t";
            print_error(L, "\t<call stack>");
        }

        if (ar.name != nullptr) {
            print_error(L, "%s%s() : line %d [%s : line %d]", indent, ar.name, ar.currentline, ar.short_src, ar.linedefined);
        } else {
            print_error(L, "%sunknown : line %d [%s : line %d]", indent, ar.currentline, ar.short_src, ar.linedefined);
        }
    }
}

void print_error(lua_State *L, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    std::printf("\n");
    va_end(args);
}

int on_error(lua_State *L)
{
    print_error(L, "%s", lua_tostring(L, -1));
    print_call_stack(L);
    return 0;
}

void enum_stack(lua_State *L)
{
    const int top = lua_gettop(L);
    print_error(L, "stack: %d", top);

    for (int i = 1; i <= top; ++i) {
        int type = lua_type(L, i);
        const char *name = lua_typename(L, type);
        switch (type) {
        case LUA_TNIL:
            print_error(L, "\t%s", name);
            break;
        case LUA_TBOOLEAN:
            print_error(L, "\t%s\t%s", name, lua_toboolean(L, i) ? "true" : "false");
            break;
        case LUA_TLIGHTUSERDATA:
            print_error(L, "\t%s\t%p", name, lua_topointer(L, i));
            break;
        case LUA_TNUMBER:
            print_error(L, "\t%s\t%g", name, lua_tonumber(L, i));
            break;
        case LUA_TSTRING:
            print_error(L, "\t%s\t%s", name, lua_tostring(L, i));
            break;
        case LUA_TTABLE:
            print_error(L, "\t%s\t%p", name, lua_topointer(L, i));
            break;
        case LUA_TFUNCTION:
            print_error(L, "\t%s\t%p", name, lua_topointer(L, i));
            break;
        case LUA_TUSERDATA:
            print_error(L, "\t%s\t%p", name, lua_topointer(L, i));
            break;
        case LUA_TTHREAD:
            print_error(L, "\t%s\t%p", name, lua_topointer(L, i));
            break;
        }
    }
}

static bool parent_meta_getter(lua_State *L)
{
    bool found = false;
    lua_pushliteral(L, LITERAL_PARENT);
    lua_rawget(L, -2);
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (lua_type(L, -1) == LUA_TUSERDATA) {
            lua2object<var_base*>::invoke(L, -1)->get(L);
            found = true;
        } else if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            found = parent_meta_getter(L);
        } else if (lua_isfunction(L, -1)) {
            found = true;
        } else {
            luaL_error(L, "find '%s' class variable. (why is not function type ?)", lua_tostring(L, 2));
        }
    } else if (!lua_isnil(L, -1)) {
        luaL_error(L, "find 'parent' class variable. (the kind of variable is nonsupport.)");
    }
    return found;
}

static bool parent_meta_setter(lua_State *L)
{
    bool found = false;
    lua_pushliteral(L, LITERAL_PARENT);
    lua_rawget(L, -2);
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (lua_type(L, -1) == LUA_TUSERDATA) {
            lua2object<var_base*>::invoke(L, -1)->set(L);
            found = true;
        } else if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            found = parent_meta_setter(L);
        } else {
            luaL_error(L, "find '%s' class variable. (why is not userdata type ?)", lua_tostring(L, 2));
        }
    } else if (!lua_isnil(L, -1)) {
        luaL_error(L, "find 'parent' class variable. (the kind of variable is nonsupport.)");
    }
    return found;
}

static int meta_getter(lua_State *L)
{
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);

    if (lua_type(L, -1) == LUA_TUSERDATA) {
        lua2object<var_base*>::invoke(L, -1)->get(L);
    } else if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        if (!parent_meta_getter(L)) {
            luaL_error(L, "can't find '%s' class variable. (forgot registering class variable ?)", lua_tostring(L, 2));
        }
    } else if (!lua_isfunction(L, -1)) {
        luaL_error(L, "find '%s' class variable. (why is not function type ?)", lua_tostring(L, 2));
    }

    lua_replace(L, 3);
    lua_settop(L, 3);

    return 1;
}

static int meta_setter(lua_State *L)
{
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);

    if (lua_type(L, -1) == LUA_TUSERDATA) {
        lua2object<var_base*>::invoke(L, -1)->set(L);
    } else if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        if (!parent_meta_setter(L)) {
            luaL_error(L, "can't find '%s' class variable. (forgot registering class variable ?)", lua_tostring(L, 2));
        }
    } else {
        luaL_error(L, "find '%s' class variable. (why is not userdata type ?)", lua_tostring(L, 2));
    }

    lua_settop(L, 3);

    return 0;
}

static int is_object_alive(lua_State *L)
{
    const int top = lua_gettop(L);
    if (top >= 1 && lua_type(L, -1) == LUA_TUSERDATA) {
        lua_pushboolean(L, lua2object<user*>::invoke(L, -1)->obj != nullptr);
    } else {
        luaL_error(L, "function signature is `userdata:is_object_alive()`");
    }
    return 1;
}

static int reinterpret_object(lua_State *L)
{
    const int top = lua_gettop(L);
    if (top >= 2 && lua_type(L, -2) == LUA_TUSERDATA && lua_istable(L, -1)) {
        lua_pushvalue(L, -1);
        lua_setmetatable(L, -3);
    } else {
        luaL_error(L, "function signature is `userdata:reinterpret_object(metatable)`");
    }
    return 0;
}

static int downcast_object(lua_State *L)
{
    const int top = lua_gettop(L);
    if (top >= 2 && lua_type(L, -2) == LUA_TUSERDATA && lua_istable(L, -1)) {
        lua_getmetatable(L, -2);
        if (!lua_rawequal(L, -1, -2)) {
            lua_pushvalue(L, -2);
            while (true) {
                lua_pushliteral(L, LITERAL_PARENT);
                lua_rawget(L, -2);
                if (!lua_istable(L, -1)) {
                    break;
                }
                if (lua_rawequal(L, -1, top + 1)) {
                    lua_pushvalue(L, top);
                    lua_setmetatable(L, top - 1);
                    break;
                }
            }
        }
        lua_settop(L, top);
    } else {
        luaL_error(L, "function signature is `userdata:downcast_object(metatable)`");
    }
    return 0;
}

void register_prototype(lua_State *L, const char *name)
{
    luaL_newmetatable(L, name);

    lua_pushliteral(L, "is_object_alive");
    lua_pushcfunction(L, is_object_alive);
    lua_rawset(L, -3);

    lua_pushliteral(L, "reinterpret");
    lua_pushcfunction(L, reinterpret_object);
    lua_rawset(L, -3);

    lua_pushliteral(L, "downcast");
    lua_pushcfunction(L, downcast_object);
    lua_rawset(L, -3);

    lua_pushliteral(L, "__index");
    lua_pushcfunction(L, meta_getter);
    lua_rawset(L, -3);

    lua_pushliteral(L, "__newindex");
    lua_pushcfunction(L, meta_setter);
    lua_rawset(L, -3);

    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, destroyer<void>);
    lua_rawset(L, -3);

    lua_setglobal(L, name);
}

void register_parent(lua_State *L, const char *name, const char *parent)
{
    push_metatable(L, name);
    if (lua_istable(L, -1)) {
        lua_pushliteral(L, LITERAL_PARENT);
        push_metatable(L, parent);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

bool binder::try_push_this(lua_State *pL, const char *metaname) const
{
    luaL_getmetatable(pL, LITERAL_UNIQUE);
    lua_rawgetp(pL, -1, this);
    if (lua_type(pL, -1) != LUA_TUSERDATA) {
        lua_pop(pL, 2);
        return false;
    }

    push_metatable(pL, metaname);
    lua_pop(pL, downcast_object(pL) + 1);
    lua_replace(pL, -2);
    return true;
}

void binder::bind(lua_State *pL, const char *name) const
{
    if (lua_type(pL, -1) == LUA_TUSERDATA) {
        if (user_ == nullptr || L == pL) {
            if (user_ != nullptr) {
                const_cast<void*&>(user_->obj) = nullptr;
            }
            L = pL, user_ = lua2object<user*>::invoke(pL, -1);
            register_this(pL);
        }
        else {
            print_error(pL, "'%s' perhaps appear a wild pointer %p,%p!=%p,%p.",
                        name, user_->obj, L, this, pL);
        }
    }
}

void binder::unbind(user *puser, const char *name) const
{
    if (user_ != nullptr) {
        if (user_ == puser) {
            L = nullptr, user_ = nullptr;
        } else {
            print_error(L, "'%s' perhaps appear a wild pointer %p->%p!=%p->%p.",
                        name, this, user_, puser, puser->obj);
        }
    }
}

void binder::register_this(lua_State *pL) const
{
    if (lua_type(pL, -1) == LUA_TUSERDATA) {
        luaL_getmetatable(pL, LITERAL_UNIQUE);
        lua_pushvalue(pL, -2);
        lua_rawsetp(pL, -2, this);
        lua_pop(pL, 1);
    }
}

void binder::on_destroy() const
{
    if (user_ != nullptr) {
        const_cast<void*&>(user_->obj) = nullptr;
        luaL_getmetatable(L, LITERAL_UNIQUE);
        lua_pushnil(L);
        lua_rawsetp(L, -2, this);
        lua_pop(L, 1);
    }
}

table::table_obj::table_obj(lua_State *L, int index)
: L(L)
, index_(index)
, pointer_(lua_istable(L, index) ? lua_topointer(L, index) : nullptr)
{
}

table::table_obj::~table_obj()
{
    if (is_alive()) {
        lua_remove(L, index_);
    }
}

size_t table::table_obj::len() const
{
    if (is_alive()) {
        lua_len(L, index_);
        return pop<size_t>::invoke(L);
    }
    return 0;
}

table::table(lua_State *L)
{
    lua_newtable(L);
    obj_ = std::make_shared<table_obj>(L, lua_gettop(L));
}

table::table(lua_State *L, int index)
{
    if (index < 0) {
        index += lua_gettop(L) + 1;
    }
    assert(index > 0 && "stack index is invalid.");
    obj_ = std::make_shared<table_obj>(L, index);
}

table::table(lua_State *L, const char *name)
{
    lua_getglobal(L, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, name);
    }
    obj_ = std::make_shared<table_obj>(L, lua_gettop(L));
}

}
