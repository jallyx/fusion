#include "AoiActor.h"
#include "AoiHandler.h"
#include "Macro.h"
#include "InlineFuncs.h"
#include <assert.h>

AoiActor::AoiActor(Type type, int flags)
: type_(type)
, flags_(flags)
, handler_(nullptr)
, x_(0)
, z_(0)
, r_(0)
{
    std::fill_n(limits_, ARRAY_SIZE(limits_), SIZE_MAX);
}

AoiActor::~AoiActor()
{
    assert(handler_ == nullptr && "please popup before destroy.");
}

void AoiActor::SetRadius(float r)
{
    r_ = r;
}

void AoiActor::SetLimits(const size_t (&limits)[N])
{
    std::copy_n(limits, ARRAY_SIZE(limits), limits_);
}

void AoiActor::CollateMarker()
{
    CheckPossible();
    for (int type = 0; type < N; ++type) {
        CheckOverflow(type);
    }
    if (BIT_ISSET(flags(), PRECISE)) {
        for (int type = 0; type < N; ++type) {
            CheckMarker(type);
        }
    }
}

void AoiActor::OnPush(AoiHandler *handler, float x, float z)
{
    assert(handler_ == nullptr && "can't repeat push.");
    handler_ = handler, x_ = x, z_ = z;
    handler->AddActor(this);
}

void AoiActor::OnMove(float x, float z)
{
    x_ = x, z_ = z;
    handler_->MoveActor(this);
}

void AoiActor::OnPopup()
{
    for (auto itr = holder_.begin(); itr != holder_.end();) {
        (*itr++)->LeaveArea(this);
    }

    for (auto &entities : marker_) {
        for (auto itr = entities.begin(); itr != entities.end();) {
            this->LeaveArea(*itr++);
        }
    }
    for (auto &entities : overflow_) {
        for (auto itr = entities.begin(); itr != entities.end();) {
            this->LeaveArea(*itr++);
        }
    }
    for (auto itr = possible_.begin(); itr != possible_.end();) {
        this->LeaveArea(*itr++);
    }

    handler_->RemoveActor(this);
    handler_ = nullptr;
}

void AoiActor::EnterArea(AoiActor *actor)
{
    if (this->IsInteresting(actor)) {
        if (this->IsInRadius(actor)) {
            if (marker_[actor->type()].size() < limits_[actor->type()]) {
                this->AddMarker(actor);
            } else {
                this->AddOverflow(actor);
            }
        } else {
            this->AddPossible(actor);
        }
        actor->AddHolder(this);
    }
}

void AoiActor::LeaveArea(AoiActor *actor)
{
    this->RemoveSubject(actor);
    actor->RemoveObserver(this);
}

void AoiActor::CleanObserver()
{
    auto Clean = [this](const std::unordered_set<AoiActor*> &entities) {
        for (auto itr = entities.begin(); itr != entities.end();) {
            AoiActor *actor = *itr++;
            if (!actor->IsInArea(this) || !actor->IsInteresting(this)) {
                actor->RemoveSubject(this);
                this->RemoveObserver(actor);
            }
        }
    };

    Clean(holder_);
}

void AoiActor::CleanSubject()
{
    auto Clean = [this](const std::unordered_set<AoiActor*> &entities) {
        for (auto itr = entities.begin(); itr != entities.end();) {
            AoiActor *actor = *itr++;
            if (!this->IsInArea(actor) || !this->IsInteresting(actor)) {
                this->RemoveSubject(actor);
                actor->RemoveObserver(this);
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

void AoiActor::AddSubject(AoiActor *actor)
{
    if (marker_[actor->type()].count(actor) != 0) return;
    if (overflow_[actor->type()].count(actor) != 0) return;
    if (possible_.count(actor) != 0) return;
    this->EnterArea(actor);
}

bool AoiActor::IsInteresting(AoiActor *actor) const
{
    if (r_ == 0) return false;
    if (actor == this) return false;
    if (BIT_ISSET(actor->flags(), VACUOUS))
        return false;
    if (BIT_ISSET(actor->flags(), SPAWN))
        return BIT_ISSET(flags(), VIEWER);
    return TestInteresting(actor);
}

bool AoiActor::IsInArea(AoiActor *actor) const
{
    return this->radius() != 0 &&
           this->x_left() < actor->x_center() &&
           this->x_right() > actor->x_center() &&
           this->z_front() < actor->z_center() &&
           this->z_back() > actor->z_center();
}

bool AoiActor::IsInRadius(AoiActor *actor) const
{
    return r_ != 0 && r_ * r_ > DistanceSquare(actor);
}

bool AoiActor::ForeachWatcher(const std::function<bool(AoiActor*)> &func) const
{
    for (int type = 0; type < N; ++type) {
        if (!ForeachWatcher(type, func)) {
            return false;
        }
    }
    return true;
}

bool AoiActor::ForeachWatcher(int type, const std::function<bool(AoiActor*)> &func) const
{
    const std::unordered_set<AoiActor*> &watchers = watcher_[type];
    for (auto actor : watchers) {
        if (!func(actor)) {
            return false;
        }
    }
    return true;
}

bool AoiActor::ForeachMarker(const std::function<bool(AoiActor*)> &func) const
{
    for (int type = 0; type < N; ++type) {
        if (!ForeachMarker(type, func)) {
            return false;
        }
    }
    return true;
}

bool AoiActor::ForeachMarker(int type, const std::function<bool(AoiActor*)> &func) const
{
    const std::unordered_set<AoiActor*> &markers = marker_[type];
    for (auto actor : markers) {
        if (!func(actor)) {
            return false;
        }
    }
    return true;
}

void AoiActor::AddWatcher(AoiActor *actor)
{
    watcher_[actor->type()].insert(actor);
}

void AoiActor::RemoveWatcher(AoiActor *actor)
{
    watcher_[actor->type()].erase(actor);
}

void AoiActor::AddHolder(AoiActor *actor)
{
    holder_.insert(actor);
}

void AoiActor::RemoveHolder(AoiActor *actor)
{
    holder_.erase(actor);
}

void AoiActor::AddMarker(AoiActor *actor)
{
    if (marker_[actor->type()].insert(actor).second) {
        actor->AddWatcher(this);
        if (!BIT_ISSET(actor->flags(), SPAWN)) {
            this->OnAddMarker(actor);
        } else {
            actor->OnSpawn();
        }
    }
}

void AoiActor::RemoveMarker(AoiActor *actor)
{
    if (marker_[actor->type()].erase(actor) != 0) {
        actor->RemoveWatcher(this);
        if (!BIT_ISSET(actor->flags(), SPAWN)) {
            this->OnRemoveMarker(actor);
        }
    }
}

void AoiActor::AddOverflow(AoiActor *actor)
{
    overflow_[actor->type()].insert(actor);
}

void AoiActor::RemoveOverflow(AoiActor *actor)
{
    overflow_[actor->type()].erase(actor);
}

void AoiActor::AddPossible(AoiActor *actor)
{
    possible_.insert(actor);
}

void AoiActor::RemovePossible(AoiActor *actor)
{
    possible_.erase(actor);
}

void AoiActor::RemoveObserver(AoiActor *actor)
{
    this->RemoveWatcher(actor);
    this->RemoveHolder(actor);
}

void AoiActor::RemoveSubject(AoiActor *actor)
{
    this->RemoveMarker(actor);
    this->RemoveOverflow(actor);
    this->RemovePossible(actor);
}

void AoiActor::CheckPossible()
{
    if (possible_.empty()) {
        return;
    }

    for (auto itr = possible_.begin(); itr != possible_.end();) {
        AoiActor *actor = *itr++;
        if (this->IsInRadius(actor)) {
            if (marker_[actor->type()].size() < limits_[actor->type()]) {
                this->AddMarker(actor);
            } else {
                this->AddOverflow(actor);
            }
            RemovePossible(actor);
        }
    }
}

void AoiActor::CheckMarker(int type)
{
    std::unordered_set<AoiActor*> &markers = marker_[type];
    if (markers.empty()) {
        return;
    }

    for (auto itr = markers.begin(); itr != markers.end();) {
        AoiActor *actor = *itr++;
        if (!this->IsInRadius(actor)) {
            this->RemoveMarker(actor);
            this->AddPossible(actor);
        }
    }
}

void AoiActor::CheckOverflow(int type)
{
    std::unordered_set<AoiActor*> &overflows = overflow_[type];
    if (overflows.empty()) {
        return;
    }

    std::unordered_set<AoiActor*> &markers = marker_[type];
    const size_t limits = limits_[type];
    if (markers.size() + overflows.size() <= limits) {
        for (auto itr = overflows.begin(); itr != overflows.end();) {
            AoiActor *actor = *itr++;
            this->AddMarker(actor);
            this->RemoveOverflow(actor);
        }
        return;
    }

    struct Actor {
        AoiActor *actor;
        float distance;
    };
    std::vector<Actor> vmarker;
    vmarker.reserve(markers.size());
    for (auto actor : markers) {
        vmarker.push_back(Actor{actor, DistanceSquare(actor)});
    }
    std::vector<Actor> voverflow;
    voverflow.reserve(overflows.size());
    for (auto actor : overflows) {
        voverflow.push_back(Actor{actor, DistanceSquare(actor)});
    }

    auto compare = [](const Actor &a1, const Actor &a2)->bool {
        return a1.distance < a2.distance;
    };
    std::sort(vmarker.begin(), vmarker.end(), compare);
    std::sort(voverflow.begin(), voverflow.end(), compare);
    const size_t vacancy = limits - markers.size();
    for (size_t index = 0; index < vacancy; ++index) {
        AoiActor *actor = voverflow[index].actor;
        this->AddMarker(actor);
        this->RemoveOverflow(actor);
    }
    const size_t total = voverflow.size();
    for (size_t index = vacancy; index < total && !vmarker.empty(); ++index) {
        const Actor &marker_actor = vmarker.back();
        const Actor &overflow_actor = voverflow[index];
        if (marker_actor.distance > overflow_actor.distance) {
            this->RemoveMarker(marker_actor.actor);
            this->AddMarker(overflow_actor.actor);
            this->RemoveOverflow(overflow_actor.actor);
            this->AddOverflow(marker_actor.actor);
            vmarker.pop_back();
        } else {
            break;
        }
    }
}
