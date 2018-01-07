#pragma once

#include <functional>
#include <unordered_set>

class AOIManager;

class AOIEntity
{
public:
    enum Type {Player, Creature, Other, N};
    enum Axis {X, Z};

    AOIEntity(int type, void *obj);
    ~AOIEntity();

    void SetAttribute(float x, float z, float r);
    void SetLimits(const size_t (&limits)[N]);
    void SetTestInteresting(const std::function<bool(void*)> &func);
    void SetOnAddMarker(const std::function<void(void*)> &func);
    void SetOnRemoveMarker(const std::function<void(void*)> &func);

    void Tick();

    void OnPush(AOIManager *mgr);
    void OnPopup();
    void OnMove(float x, float z);

    void EnterArea(AOIEntity *entity);
    void LeaveArea(AOIEntity *entity);

    void CleanObserver();
    void CleanSubject();
    void AddSubject(AOIEntity *entity);

    bool IsInteresting(AOIEntity *entity) const;
    bool IsInArea(AOIEntity *entity) const;
    bool IsInRadius(AOIEntity *entity) const;

    bool ForeachWatcher(const std::function<bool(void*)> &func) const;
    bool ForeachWatcher(int type, const std::function<bool(void*)> &func) const;
    bool ForeachMarker(const std::function<bool(void*)> &func) const;
    bool ForeachMarker(int type, const std::function<bool(void*)> &func) const;

    bool IsPushMgr() const { return mgr_ != nullptr; }

    float x_left() const { return x_ - r_; }
    float x_right() const { return x_ + r_; }
    float z_front() const { return z_ - r_; }
    float z_back() const { return z_ + r_; }
    float x_center() const { return x_; }
    float z_center() const { return z_; }
    float radius() const { return r_; }

    int type() const { return type_; }
    void *object() const { return obj_; }

private:
    void AddWatcher(AOIEntity *entity);
    void RemoveWatcher(AOIEntity *entity);
    void AddHolder(AOIEntity *entity);
    void RemoveHolder(AOIEntity *entity);

    void AddMarker(AOIEntity *entity);
    void RemoveMarker(AOIEntity *entity);
    void AddOverflow(AOIEntity *entity);
    void RemoveOverflow(AOIEntity *entity);
    void AddPossible(AOIEntity *entity);
    void RemovePossible(AOIEntity *entity);

    void RemoveObserver(AOIEntity *entity);
    void RemoveSubject(AOIEntity *entity);

    void CheckPossible();
    void CheckOverflow(int type);

    float DistanceSquare(AOIEntity *entity) const {
        float x_delta = entity->x_center() - this->x_center();
        float z_delta = entity->z_center() - this->z_center();
        return x_delta * x_delta + z_delta * z_delta;
    }

    int const type_;
    void * const obj_;

    AOIManager *mgr_;
    float x_, z_, r_;

    std::function<bool(void*)> test_interesting_;
    std::function<void(void*)> on_add_marker_;
    std::function<void(void*)> on_remove_marker_;

    std::unordered_set<AOIEntity*> watcher_[N];
    std::unordered_set<AOIEntity*> holder_;

    std::unordered_set<AOIEntity*> marker_[N];
    std::unordered_set<AOIEntity*> overflow_[N];
    std::unordered_set<AOIEntity*> possible_;

    size_t limits_[N];
};
