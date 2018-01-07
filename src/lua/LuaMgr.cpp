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

bool LuaMgr::DoFile(lua_State *L, const std::string &fileName)
{
    std::string chunk;
    bool isCached = false;
    do {
        rwlock_.rdlock();
        std::lock_guard<rwlock> lock(rwlock_, std::adopt_lock);
        auto iterator = chunks_.find(fileName);
        if (iterator != chunks_.end()) {
            chunk = iterator->second;
            isCached = true;
        }
    } while (0);

    lua_pushcfunction(L, lua::on_error);
    int errfunc = lua_gettop(L);
    int errcode = isCached ?
        luaL_loadbuffer(L, chunk.data(), chunk.size(), fileName.c_str()) :
        luaL_loadfile(L, fileName.c_str());
    if (errcode == 0) {
        if (!isCached) {
            DumpChunk(L, fileName);
        }
        lua_pcall(L, 0, 1, errfunc);
    } else {
        ELOG("dofile[%s] error: %s.", fileName.c_str(), lua_tostring(L, -1));
    }
    lua_pop(L, 2);

    return errcode == 0;
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
