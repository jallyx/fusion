#pragma once

#include <list>
#include <unordered_map>

class AoiActor;

class AoiHandler
{
public:
    AoiHandler();
    ~AoiHandler();

    void AddActor(AoiActor *actor);
    void MoveActor(AoiActor *actor);
    void RemoveActor(AoiActor *actor);

    void ReloadActorRadius(AoiActor *actor);

    void ReloadActorObserver(AoiActor *actor) const;
    void ReloadActorSubject(AoiActor *actor) const;
    void ReloadActorStatus(AoiActor *actor) const;

    void ReloadAllActorSubject() const;

    void CollateAllActorMarker() const;

    enum StableStatus { SS_None, SS_Read, SS_Write };
    StableStatus stable_status() const { return stable_status_; }

private:
    static float GetNodeValue(AoiActor *actor, int axis, int anchor);

    enum Anchor {Begin, Center, End, N};
    struct Node {
        AoiActor *actor;
        Anchor anchor;
        float value;
    };
    struct Iterator {
        std::list<Node>::iterator xitr[N];
        std::list<Node>::iterator zitr[N];
    };

    std::list<Node> x_link_;
    std::list<Node> z_link_;
    std::unordered_map<AoiActor*, Iterator> itrs_;

    class AutoStableStatus;
    mutable StableStatus stable_status_;
};
