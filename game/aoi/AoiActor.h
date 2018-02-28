#pragma once

#include <functional>
#include <unordered_set>

class AoiHandler;

class AoiActor
{
public:
    enum Flag {PRECISE, VACUOUS, SPAWN, VIEWER};
    enum Type {PLAYER, CREATURE, SOBJ, OTHER, N};
    enum Axis {X, Z};

    AoiActor(Type type, int flags);
    ~AoiActor();

    void SetRadius(float r);
    void SetLimits(const size_t (&limits)[N]);

    void CollateMarker();

    void OnPush(AoiHandler *handler, float x, float z);
    void OnMove(float x, float z);
    void OnPopup();

    void EnterArea(AoiActor *actor);
    void LeaveArea(AoiActor *actor);

    void CleanObserver();
    void CleanSubject();
    void AddSubject(AoiActor *actor);

    bool IsInteresting(AoiActor *actor) const;
    bool IsInArea(AoiActor *actor) const;
    bool IsInRadius(AoiActor *actor) const;

    bool ForeachWatcher(const std::function<bool(AoiActor*)> &func) const;
    bool ForeachWatcher(int type, const std::function<bool(AoiActor*)> &func) const;
    bool ForeachMarker(const std::function<bool(AoiActor*)> &func) const;
    bool ForeachMarker(int type, const std::function<bool(AoiActor*)> &func) const;

    size_t GetWatcherCount(int type) const { return watcher_[type].size(); }
    size_t GetMarkerCount(int type) const { return marker_[type].size(); }

    bool IsInWorld() const { return handler_ != nullptr; }

    float x_left() const { return x_ - r_; }
    float x_right() const { return x_ + r_; }
    float z_front() const { return z_ - r_; }
    float z_back() const { return z_ + r_; }
    float x_center() const { return x_; }
    float z_center() const { return z_; }
    float radius() const { return r_; }

protected:
    virtual bool TestInteresting(AoiActor *actor) const { return false; }
    virtual void OnAddMarker(AoiActor *actor) {}
    virtual void OnRemoveMarker(AoiActor *actor) {}
    virtual void OnSpawn() {}

private:
    void AddWatcher(AoiActor *actor);
    void RemoveWatcher(AoiActor *actor);
    void AddHolder(AoiActor *actor);
    void RemoveHolder(AoiActor *actor);

    void AddMarker(AoiActor *actor);
    void RemoveMarker(AoiActor *actor);
    void AddOverflow(AoiActor *actor);
    void RemoveOverflow(AoiActor *actor);
    void AddPossible(AoiActor *actor);
    void RemovePossible(AoiActor *actor);

    void RemoveObserver(AoiActor *actor);
    void RemoveSubject(AoiActor *actor);

    void CheckPossible();
    void CheckMarker(int type);
    void CheckOverflow(int type);

    float DistanceSquare(AoiActor *actor) const {
        float x_delta = actor->x_center() - this->x_center();
        float z_delta = actor->z_center() - this->z_center();
        return x_delta * x_delta + z_delta * z_delta;
    }

    Type type() const { return type_; }
    int flags() const { return flags_; }

    const Type type_;
    const int flags_;

    AoiHandler *handler_;
    float x_, z_, r_;

    std::unordered_set<AoiActor*> watcher_[N];
    std::unordered_set<AoiActor*> holder_;

    std::unordered_set<AoiActor*> marker_[N];
    std::unordered_set<AoiActor*> overflow_[N];
    std::unordered_set<AoiActor*> possible_;

    size_t limits_[N];
};
