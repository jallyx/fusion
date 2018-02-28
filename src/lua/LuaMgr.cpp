#include "LuaMgr.h"
#include "LuaBus.h"
#include "Logger.h"
#include <sstream>

LuaMgr::LuaMgr()
{
}

LuaMgr::~LuaMgr()
{
}

int LuaMgr::LoadFile(lua_State *L, const std::string &fileName)
{
    int errcode = CacheFile(L, fileName);
    if (errcode != 0) {
        ELOG("LoadFile[%s] error: %s.", fileName.c_str(), lua_tostring(L, -1));
        lua_pop(L, 1);
        lua_pushnil(L);
    }
    return 1;
}

bool LuaMgr::DoFile(lua_State *L, const std::string &fileName)
{
    lua_pushcfunction(L, lua::on_error);
    int errfunc = lua_gettop(L);
    int errcode = CacheFile(L, fileName);
    if (errcode == 0) {
        lua_pcall(L, 0, 1, errfunc);
    } else {
        ELOG("DoFile[%s] error: %s.", fileName.c_str(), lua_tostring(L, -1));
    }
    lua_pop(L, 2);
    return errcode == 0;
}

void LuaMgr::DoFileEnv(lua_State *L, const char *fileName)
{
    if (LuaMgr::instance() != nullptr) {
        sLuaMgr.DoFile(L, fileName);
    } else {
        lua::dofile(L, fileName);
    }
}

int LuaMgr::CacheFile(lua_State *L, const std::string &fileName)
{
    std::string chunk;
    do {
        rwlock_.rdlock();
        std::lock_guard<rwlock> lock(rwlock_, std::adopt_lock);
        auto itr = chunks_.find(fileName);
        if (itr != chunks_.end()) {
            chunk = itr->second;
        }
    } while (0);

    int errcode = !chunk.empty() ?
        luaL_loadbuffer(L, chunk.data(), chunk.size(), fileName.c_str()) :
        luaL_loadfile(L, fileName.c_str());
    if (errcode == 0) {
        if (chunk.empty()) {
            DumpChunk(L, fileName);
        }
    }

    return errcode;
}

void LuaMgr::DumpChunk(lua_State *L, const std::string &fileName)
{
    std::ostringstream stream;
    lua_dump(L, (lua_Writer)WriteChunk, &stream, 0);
    do {
        rwlock_.wrlock();
        std::lock_guard<rwlock> lock(rwlock_, std::adopt_lock);
        chunks_[fileName] = stream.str();
    } while (0);
}

int LuaMgr::WriteChunk(lua_State *L, const char *p, size_t sz, std::ostream *stream)
{
    (*stream).write(p, sz);
    return 0;
}
