#include "ai/AIActionNode.h"
#include "ai/AIBehaviorTree.h"
#include "ServerMaster.h"
#include "Logger.h"
#include "OS.h"
#include "System.h"

class SCVBlackboard : public AIBlackboard
{
public:
    SCVBlackboard(GameObject *game_object) : AIBlackboard(game_object) {}
    virtual ~SCVBlackboard() {}
};

class GameObject
{
public:
    GameObject(AIBlackboard &bb) : tree(bb) {}
    virtual ~GameObject() {}

    AIBehaviorTree tree;
};

class SCVObject : public GameObject
{
public:
    SCVObject(lua_State *L) : GameObject(bb), bb(this) {
        tree.SetGameObjectMeta<SCVObject>(L);
    }
    virtual ~SCVObject() {}

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

    SCVBlackboard bb;
};

class MoveToBase : public AIActionNode
{
public:
    MoveToBase(AIBlackboard &blackboard, int speed)
        : AIActionNode(blackboard), speed_(speed)
    {}
    Status Kernel() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->pos -= speed_;
        NLOG("move to base %d ...", pscv->pos);
        return pscv->IsNearBase() ? Finished : Running;
    }
    int speed_;
};
class MoveToRes : public AIActionNode
{
public:
    MoveToRes(AIBlackboard &blackboard, int speed)
        : AIActionNode(blackboard), speed_(speed)
    {}
    Status Kernel() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->pos += speed_;
        NLOG("move to res %d ...", pscv->pos);
        return pscv->IsNearRes() ? Finished : Running;
    }
    int speed_;
};
class StartReturnRes : public AIActionNode
{
public:
    StartReturnRes(AIBlackboard &blackboard)
        : AIActionNode(blackboard)
    {}
    Status Kernel() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->status = SCVObject::Returning;
        NLOG("start return res ...");
        return Finished;
    }
};
class StartGatherRes : public AIActionNode
{
public:
    StartGatherRes(AIBlackboard &blackboard)
        : AIActionNode(blackboard)
    {}
    Status Kernel() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->status = SCVObject::Gathering;
        NLOG("start gather res ...");
        return Finished;
    }
};
class SelectRes : public AIActionNode
{
public:
    SelectRes(AIBlackboard &blackboard)
        : AIActionNode(blackboard)
    {}
    Status Kernel() {
        NLOG("select res ...");
        return Finished;
    }
};
class ReturnRes : public AIActionNode
{
public:
    ReturnRes(AIBlackboard &blackboard)
        : AIActionNode(blackboard)
    {}
    Status Kernel() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->res -= 30;
        NLOG("return res %d ...", pscv->res);
        return pscv->IsEmptyRes() ? Finished : Running;
    }
    void OnFinish() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->status = SCVObject::Invalid;
    }
};
class GatherRes : public AIActionNode
{
public:
    GatherRes(AIBlackboard &blackboard, int number)
        : AIActionNode(blackboard), number_(number)
    {}
    Status Kernel() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->res += number_;
        NLOG("gather res %d ...", pscv->res);
        return pscv->IsFullRes() ? Finished : Running;
    }
    void OnFinish() {
        SCVObject *pscv = dynamic_cast<SCVObject*>(blackboard_.game_object());
        pscv->status = SCVObject::Invalid;
    }
    int number_;
};
class Sing : public AIActionNode
{
public:
    Sing(AIBlackboard &blackboard)
        : AIActionNode(blackboard)
    {}
    Status Kernel() {
        NLOG("sing ...");
        return Running;
    }
};

void initSCVAIMeta(lua_State *L)
{
    lua::class_add<GameObject>(L, "GameObject");
    lua::class_add<SCVObject>(L, "SCVObject");
    lua::class_inh<SCVObject, GameObject>(L);
    lua::class_def<SCVObject>(L, "IsEmptyRes", &SCVObject::IsEmptyRes);
    lua::class_def<SCVObject>(L, "IsFullRes", &SCVObject::IsFullRes);
    lua::class_def<SCVObject>(L, "IsGatheringRes", &SCVObject::IsGatheringRes);
    lua::class_def<SCVObject>(L, "IsReturningRes", &SCVObject::IsReturningRes);
    lua::class_def<SCVObject>(L, "IsNearBase", &SCVObject::IsNearBase);
    lua::class_def<SCVObject>(L, "IsNearRes", &SCVObject::IsNearRes);

    lua::class_add<MoveToBase>(L, "MoveToBase");
    lua::class_con<MoveToBase>(L, lua::constructor<MoveToBase, AIBlackboard&, int>);
    lua::class_inh<MoveToBase, AIActionNode>(L);
    lua::class_add<MoveToRes>(L, "MoveToRes");
    lua::class_con<MoveToRes>(L, lua::constructor<MoveToRes, AIBlackboard&, int>);
    lua::class_inh<MoveToRes, AIActionNode>(L);
    lua::class_add<StartReturnRes>(L, "StartReturnRes");
    lua::class_con<StartReturnRes>(L, lua::constructor<StartReturnRes, AIBlackboard&>);
    lua::class_inh<StartReturnRes, AIActionNode>(L);
    lua::class_add<StartGatherRes>(L, "StartGatherRes");
    lua::class_con<StartGatherRes>(L, lua::constructor<StartGatherRes, AIBlackboard&>);
    lua::class_inh<StartGatherRes, AIActionNode>(L);
    lua::class_add<SelectRes>(L, "SelectRes");
    lua::class_con<SelectRes>(L, lua::constructor<SelectRes, AIBlackboard&>);
    lua::class_inh<SelectRes, AIActionNode>(L);
    lua::class_add<ReturnRes>(L, "ReturnRes");
    lua::class_con<ReturnRes>(L, lua::constructor<ReturnRes, AIBlackboard&>);
    lua::class_inh<ReturnRes, AIActionNode>(L);
    lua::class_add<GatherRes>(L, "GatherRes");
    lua::class_con<GatherRes>(L, lua::constructor<GatherRes, AIBlackboard&, int>);
    lua::class_inh<GatherRes, AIActionNode>(L);
    lua::class_add<Sing>(L, "Sing");
    lua::class_con<Sing>(L, lua::constructor<Sing, AIBlackboard&>);
    lua::class_inh<Sing, AIActionNode>(L);
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
        AIBehaviorTree::InitBehaviorTree(L);
        initSCVAIMeta(L);
        scv = new SCVObject(L);
        lua::dofile(L, "scv.lua");
        lua::call<void>(L, "BuildBehaviorTree", &scv->tree);
        return true;
    }
    virtual void StopServices() {
        delete scv;
        lua::close(L);
    }
    virtual void Tick() {
        scv->tree.Update();
        System::Update();
    }
    virtual std::string GetConfigFile() { return "config"; }
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
