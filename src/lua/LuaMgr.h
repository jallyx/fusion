#pragma once

#include "Singleton.h"
#include "Concurrency.h"
#include <lua.hpp>
#include <unordered_map>

class LuaMgr : public Singleton<LuaMgr>
{
public:
    LuaMgr();
    virtual ~LuaMgr();

    int LoadFile(lua_State *L, const std::string &fileName);

    bool DoFile(lua_State *L, const std::string &fileName);

    static void DoFileEnv(lua_State *L, const char *fileName);

private:
    int CacheFile(lua_State *L, const std::string &fileName);
    void DumpChunk(lua_State *L, const std::string &fileName);
    static int WriteChunk(lua_State *L, const char *p, size_t sz, std::ostream *stream);

    rwlock rwlock_;
    std::unordered_map<std::string, std::string> chunks_;
};

#define sLuaMgr (*LuaMgr::instance())
