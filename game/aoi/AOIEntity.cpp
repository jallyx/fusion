#include "AOIEntity.h"
#include "AOIManager.h"
#include "Macro.h"
#include <assert.h>
#include <string.h>
#include <algorithm>
#include <vector>

AOIEntity::AOIEntity(int type, void *obj)
: type_(type)
, obj_(obj)
, mgr_(nullptr)
, x_(0)
, z_(0)
, r_(0)
{
    std::fill_n(limits_, ARRAY_SIZE(limits_), SIZE_MAX);
}

AOIEntity::~AOIEntity()
{
    assert(mgr_ == nullptr && "please popup before destroy.");
}

void AOIEntity::SetAttribute(float x, float z, float r)
{
    x_ = x;
    z_ = z;
    r_ = r;
}

void AOIEntity::SetLimits(const size_t (&limits)[N])
{
    memcpy(limits_, limits, sizeof(limits));
}

void AOIEntity::SetTestInteresting(const std::function<bool(void*)> &func)
{
    test_interesting_ = func;
}

void AOIEntity::SetOnAddMarker(const std::function<void(void*)> &func)
{
    on_add_marker_ = func;
}

void AOIEntity::SetOnRemoveMarker(const std::function<void(void*)> &func)
{
    on_remove_marker_ = func;
}

void AOIEntity::Tick()
{
    CheckPossible();
    for (int type = 0; type < N; ++type) {
        CheckOverflow(type);
    }
}

void AOIEntity::OnPush(AOIManager *mgr)
{
    assert(mgr_ == nullptr && "can't repeat push.");
    mgr_ = mgr;
    mgr->AddEntity(this);
}

void AOIEntity::OnPopup()
{
    for (auto entity : holder_) {
        entity->RemoveSubject(this);
    }
    holder_.clear();
    for (auto &entities : watcher_) {
        entities.clear();
    }

    for (auto &entities : marker_) {
        for (auto entity : entities) {
            entity->RemoveObserver(this);
        }
        entities.clear();
    }
    for (auto &entities : overflow_) {
        for (auto entity : entities) {
            entity->RemoveObserver(this);
        }
        entities.clear();
    }
    for (auto entity : possible_) {
        entity->RemoveObserver(this);
    }
    possible_.clear();

    mgr_->RemoveEntity(this);
    mgr_ = nullptr;
}

void AOIEntity::OnMove(float x, float z)
{
    x_ = x;
    z_ = z;
    mgr_->MoveEntity(this);
}

void AOIEntity::EnterArea(AOIEntity *entity)
{
    if (IsInteresting(entity)) {
        if (IsInRadius(entity)) {
            if (marker_[entity->type()].size() < limits_[entity->type()]) {
                AddMarker(entity);
            } else {
                AddOverflow(entity);
            }
        } else {
            AddPossible(entity);
        }
        entity->AddHolder(this);
    }
}

void AOIEntity::LeaveArea(AOIEntity *entity)
{
    this->RemoveSubject(entity);
    entity->RemoveObserver(this);
}

void AOIEntity::CleanObserver()
{
    for (auto itr = holder_.begin(); itr != holder_.end();) {
        AOIEntity *entity = *itr; ++itr;
        if (!entity->IsInArea(this) || !entity->IsInteresting(this)) {
            entity->RemoveSubject(this);
            this->RemoveObserver(entity);
        }
    }
}

void AOIEntity::CleanSubject()
{
    auto Clean = [this](const std::unordered_set<AOIEntity*> &entities) {
        for (auto itr = entities.begin(); itr != entities.end();) {
            AOIEntity *entity = *itr; ++itr;
            if (!IsInArea(entity) || !IsInteresting(entity)) {
                RemoveSubject(entity);
                entity->RemoveObserver(this);
            }
        }
    };

    for (auto &entities : marker_) {
        Clean(entities);
    }
    for (auto &entities : overflow_) {
        Clean(entities);
    }
    Clean(possible_);
}

void AOIEntity::AddSubject(AOIEntity *entity)
{
    if (marker_[entity->type()].count(entity) != 0) return;
    if (overflow_[entity->type()].count(entity) != 0) return;
    if (possible_.count(entity) != 0) return;
    EnterArea(entity);
}

bool AOIEntity::IsInteresting(AOIEntity *entity) const
{
    if (entity == this) return false;
    if (r_ == 0) return false;
    if (!test_interesting_) return false;
    return test_interesting_(entity->object());
}

bool AOIEntity::IsInArea(AOIEntity *entity) const
{
    return this->radius() != 0 &&
           this->x_left() < entity->x_center() &&
           this->x_right() > entity->x_center() &&
           this->z_front() < entity->z_center() &&
           this->z_back() > entity->z_center();
}

bool AOIEntity::IsInRadius(AOIEntity *entity) const
{
    return r_ != 0 && r_ * r_ > DistanceSquare(entity);
}

bool AOIEntity::ForeachWatcher(const std::function<bool(void*)> &func) const
{
    for (int type = 0; type < N; ++type) {
        if (!ForeachWatcher(type, func)) {
            return false;
        }
    }
    return true;
}

bool AOIEntity::ForeachWatcher(int type, const std::function<bool(void*)> &func) const
{
    const std::unordered_set<AOIEntity*> &watchers = watcher_[type];
    for (auto entity : watchers) {
        if (!func(entity->object())) {
            return false;
        }
    }
    return true;
}

bool AOIEntity::ForeachMarker(const std::function<bool(void*)> &func) const
{
    for (int type = 0; type < N; ++type) {
        if (!ForeachMarker(type, func)) {
            return false;
        }
    }
    return true;
}

bool AOIEntity::ForeachMarker(int type, const std::function<bool(void*)> &func) const
{
    const std::unordered_set<AOIEntity*> &markers = marker_[type];
    for (auto entity : markers) {
        if (!func(entity->object())) {
            return false;
        }
    }
    return true;
}

void AOIEntity::AddWatcher(AOIEntity *entity)
{
    watcher_[entity->type()].insert(entity);
}

void AOIEntity::RemoveWatcher(AOIEntity *entity)
{
    watcher_[entity->type()].erase(entity);
}

void AOIEntity::AddHolder(AOIEntity *entity)
{
    holder_.insert(entity);
}

void AOIEntity::RemoveHolder(AOIEntity *entity)
{
    holder_.erase(entity);
}

void AOIEntity::AOIEntity::AddMarker(AOIEntity *entity)
{
    if (marker_[entity->type()].insert(entity).second) {
        entity->AddWatcher(this);
        if (on_add_marker_) {
            on_add_marker_(entity->object());
        }
    }
}

void AOIEntity::AOIEntity::RemoveMarker(AOIEntity *entity)
{
    if (marker_[entity->type()].erase(entity) != 0) {
        entity->RemoveWatcher(this);
        if (on_remove_marker_) {
            on_remove_marker_(entity->object());
        }
    }
}

void AOIEntity::AddOverflow(AOIEntity *entity)
{
    overflow_[entity->type()].insert(entity);
}

void AOIEntity::RemoveOverflow(AOIEntity *entity)
{
    overflow_[entity->type()].erase(entity);
}

void AOIEntity::AddPossible(AOIEntity *entity)
{
    possible_.insert(entity);
}

void AOIEntity::RemovePossible(AOIEntity *entity)
{
    possible_.erase(entity);
}

void AOIEntity::RemoveObserver(AOIEntity *entity)
{
    RemoveWatcher(entity);
    RemoveHolder(entity);
}

void AOIEntity::RemoveSubject(AOIEntity *entity)
{
    RemoveMarker(entity);
    RemoveOverflow(entity);
    RemovePossible(entity);
}

void AOIEntity::CheckPossible()
{
    if (possible_.empty()) {
        return;
    }

    for (auto itr = possible_.begin(); itr != possible_.end();) {
        AOIEntity *entity = *itr;
        if (IsInRadius(entity)) {
            if (marker_[entity->type()].size() < limits_[entity->type()]) {
                AddMarker(entity);
            } else {
                AddOverflow(entity);
            }
            itr = possible_.erase(itr);
        } else {
            ++itr;
        }
    }
}

void AOIEntity::CheckOverflow(int type)
{
    std::unordered_set<AOIEntity*> &overflows = overflow_[type];
    if (overflows.empty()) {
        return;
    }

    std::unordered_set<AOIEntity*> &markers = marker_[type];
    const size_t limits = limits_[type];
    if (markers.size() + overflows.size() <= limits) {
        for (auto entity : overflows) {
            AddMarker(entity);
        }
        overflows.clear();
        return;
    }

    struct Entity {
        AOIEntity *entity;
        float distance;
    };
    std::vector<Entity> vmarker;
    vmarker.reserve(markers.size());
    for (auto entity : markers) {
        vmarker.push_back(Entity{entity, DistanceSquare(entity)});
    }
    std::vector<Entity> voverflow;
    voverflow.reserve(overflows.size());
    for (auto entity : overflows) {
        voverflow.push_back(Entity{entity, DistanceSquare(entity)});
    }

    auto compare = [](const Entity &e1, const Entity &e2)->bool {
        return e1.distance < e2.distance;
    };
    std::sort(vmarker.begin(), vmarker.end(), compare);
    std::sort(voverflow.begin(), voverflow.end(), compare);
    const size_t vacancy = limits - markers.size();
    for (size_t index = 0; index < vacancy; ++index) {
        AOIEntity *entity = voverflow[index].entity;
        AddMarker(entity);
        RemoveOverflow(entity);
    }
    const size_t total = voverflow.size();
    for (size_t index = vacancy; index < total && !vmarker.empty(); ++index) {
        const Entity &marker_entity = vmarker.back();
        const Entity &overflow_entity = voverflow[index];
        if (marker_entity.distance > overflow_entity.distance) {
            RemoveMarker(marker_entity.entity);
            AddMarker(overflow_entity.entity);
            RemoveOverflow(overflow_entity.entity);
            AddOverflow(marker_entity.entity);
            vmarker.pop_back();
        } else {
            break;
        }
    }
}
