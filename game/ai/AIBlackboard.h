#pragma once

#include <functional>
#include "noncopyable.h"
#include "lua/LuaRef.h"

class AIObject;

class AIBlackboard : public lua::binder, public noncopyable
{
public:
    AIBlackboard();
    ~AIBlackboard();

    template <typename T>
    void SetObject(lua_State *L, T *object) {
        lua::push<T*>::invoke(L, object);
        ai_object_cache_ = LuaRef(L, true);
        ai_object_ = object;
    }

    AIObject *object() const { return ai_object_; }
    const LuaRef &object_cache() const { return ai_object_cache_; }

    std::function<void(lua_State *L, int index)> RefTreeNode;

private:
    AIObject *ai_object_;
    LuaRef ai_object_cache_;
};
