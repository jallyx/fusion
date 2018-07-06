#include "ai/AIObject.h"
#include "lua/LuaTable.h"
#include "ServerMaster.h"
#include "Logger.h"
#include "OS.h"
#include "System.h"

class SCVBlackboard : public AIBlackboard
{
public:
    SCVBlackboard() {}
    ~SCVBlackboard() {}
};

class SCVObject : public AIObject
{
public:
    SCVObject(lua_State *L) {
        bb.SetObject(L, this);
    }
    virtual ~SCVObject() {}

    virtual AIBlackboard &GetBlackboard() { return bb; }

    void PrintMessage(const char *msg) { NLOG("%s", msg); }

    bool IsEmptyRes() const { return res <= 0; }
    bool IsFullRes() const { return res >= fullRes; }
    bool IsNearBase() const { return pos <= basePos; }
    bool IsNearRes() const { return pos >= resPos; }
    bool IsGatheringRes() const { return status == Gathering; }
    bool IsReturningRes() const { return status == Returning; }

    const int basePos = 0;
    const int resPos = 100;
    const int fullRes = 100;
    int pos = 0;
    int res = 0;

    enum Status {
        Invalid,
        Gathering,
        Returning,
    };
    Status status = Invalid;

private:
    SCVBlackboard bb;
};

void initSCVAIMeta(lua_State *L)
{
    LuaTable t(L, "SCVStatus");
    t.set("Invalid", SCVObject::Invalid);
    t.set("Gathering", SCVObject::Gathering);
    t.set("Returning", SCVObject::Returning);

    lua::class_add<SCVObject>(L, "SCVObject");
    lua::class_inh<SCVObject, AIObject>(L);
    lua::class_def<SCVObject>(L, "PrintMessage", &SCVObject::PrintMessage);
    lua::class_def<SCVObject>(L, "IsEmptyRes", &SCVObject::IsEmptyRes);
    lua::class_def<SCVObject>(L, "IsFullRes", &SCVObject::IsFullRes);
    lua::class_def<SCVObject>(L, "IsGatheringRes", &SCVObject::IsGatheringRes);
    lua::class_def<SCVObject>(L, "IsReturningRes", &SCVObject::IsReturningRes);
    lua::class_def<SCVObject>(L, "IsNearBase", &SCVObject::IsNearBase);
    lua::class_def<SCVObject>(L, "IsNearRes", &SCVObject::IsNearRes);
    lua::class_mem<SCVObject>(L, "pos", &SCVObject::pos);
    lua::class_mem<SCVObject>(L, "res", &SCVObject::res);
    lua::class_mem<SCVObject>(L, "status", &SCVObject::status);

    lua::dofile(L, "scvnode.lua");
}

class AIServerMaster : public IServerMaster, public Singleton<AIServerMaster> {
public:
    virtual bool InitSingleton() {
        return true;
    }
    virtual void FinishSingleton() {
    }
protected:
    virtual bool InitDBPool() { return true; }
    virtual bool LoadDBData() { return true; }
    virtual bool StartServices() {
        L = lua::open();
        AIObject::InitBehaviorTree(L);
        initSCVAIMeta(L);
        scv = new SCVObject(L);
        scv->BuildBehaviorTree(L, "scv.lua");
        return true;
    }
    virtual void StopServices() {
        delete scv;
        lua::close(L);
    }
    virtual void Tick() {
        scv->RunBehaviorTree();
        System::Update();
    }
    virtual std::string GetConfigFile() { return "config"; }
    virtual size_t GetAsyncServiceCount() { return 0; }
    virtual size_t GetIOServiceCount() { return 0; }
private:
    lua_State *L;
    SCVObject *scv;
};
#define sAIServerMaster (*AIServerMaster::instance())

void AIMain(int argc, char **argv)
{
    AIServerMaster::newInstance();
    sAIServerMaster.InitSingleton();
    sAIServerMaster.Initialize(argc, argv);
    sAIServerMaster.Run(argc, argv);
    AIServerMaster::deleteInstance();
}
