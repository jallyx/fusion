#include "AoiHandler.h"
#include "AoiActor.h"
#include "Debugger.h"
#include <algorithm>

class AoiHandler::AutoStableStatus {
public:
    AutoStableStatus(const AoiHandler *handler, AoiHandler::StableStatus status)
    : handler_(handler) { handler_->stable_status_ = status; }
    ~AutoStableStatus()
    { handler_->stable_status_ = AoiHandler::SS_None; }
private:
    const AoiHandler *handler_;
};

AoiHandler::AoiHandler()
: stable_status_(SS_None)
{
}

AoiHandler::~AoiHandler()
{
}

void AoiHandler::AddActor(AoiActor *actor)
{
    auto rst = itrs_.insert(std::make_pair(actor, Iterator()));
    if (!rst.second) {
        assert(false && "can't repeat add actor.");
        return;
    }

    auto AddActorLinkNode = [actor](AoiActor::Axis axis,
        std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        Node b_node{actor, Begin, GetNodeValue(actor, axis, Begin)};
        auto itr = link.begin();
        for (; itr != link.end(); ++itr) {
            const Node &check_node = *itr;
            if (check_node.value < b_node.value) {
                if (check_node.anchor == Begin) {
                    if (check_node.actor->IsInArea(actor)) {
                        check_node.actor->EnterArea(actor);
                    }
                }
            } else {
                break;
            }
        }
        itrs[Begin] = link.insert(itr, b_node);

        if (actor->radius() != 0) {
            Node c_node{actor, Center, GetNodeValue(actor, axis, Center)};
            for (; itr != link.end(); ++itr) {
                const Node &check_node = *itr;
                if (check_node.value < c_node.value) {
                    if (check_node.anchor == Begin) {
                        if (check_node.actor->IsInArea(actor)) {
                            check_node.actor->EnterArea(actor);
                        }
                    } else if (check_node.anchor == Center) {
                        if (actor->IsInArea(check_node.actor)) {
                            actor->EnterArea(check_node.actor);
                        }
                    }
                } else {
                    break;
                }
            }
            itrs[Center] = link.insert(itr, c_node);

            Node e_node{actor, End, GetNodeValue(actor, axis, End)};
            for (; itr != link.end(); ++itr) {
                const Node &check_node = *itr;
                if (check_node.value < e_node.value) {
                    if (check_node.anchor == Center) {
                        if (actor->IsInArea(check_node.actor)) {
                            actor->EnterArea(check_node.actor);
                        }
                    }
                } else {
                    break;
                }
            }
            itrs[End] = link.insert(itr, e_node);
        } else {
            itrs[Begin]->anchor = Center;
            itrs[End] = itrs[Center] = itrs[Begin];
        }
    };

    auto AddActorLinkNodeFast = [actor](AoiActor::Axis axis,
        std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        Node b_node{actor, Begin, GetNodeValue(actor, axis, Begin)};
        auto itr = link.begin();
        for (; itr != link.end() && itr->value < b_node.value; ++itr);
        itrs[Begin] = link.insert(itr, b_node);
        if (actor->radius() != 0) {
            Node c_node{actor, Center, GetNodeValue(actor, axis, Center)};
            for (; itr != link.end() && itr->value < c_node.value; ++itr);
            itrs[Center] = link.insert(itr, c_node);
            Node e_node{actor, End, GetNodeValue(actor, axis, End)};
            for (; itr != link.end() && itr->value < e_node.value; ++itr);
            itrs[End] = link.insert(itr, e_node);
        } else {
            itrs[Begin]->anchor = Center;
            itrs[End] = itrs[Center] = itrs[Begin];
        }
    };

    DBGASSERT(stable_status_ == SS_None);
    AutoStableStatus autoStableStatus(this, SS_Write);

    Iterator &itrs = rst.first->second;
    AddActorLinkNode(AoiActor::X, x_link_, itrs.xitr);
    AddActorLinkNodeFast(AoiActor::Z, z_link_, itrs.zitr);
}

void AoiHandler::MoveActor(AoiActor *actor)
{
    auto itr = itrs_.find(actor);
    if (itr == itrs_.end()) {
        return;
    }

    auto MoveActorLinkNode = [actor](AoiActor::Axis axis,
        std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        if (itrs[Begin]->anchor == Begin) {
            std::list<Node>::iterator itr = itrs[Begin];
            Node &b_node = *itr;
            const float b_node_value = b_node.value;
            b_node.value = GetNodeValue(actor, axis, Begin);
            if (b_node_value > b_node.value) {
                for (; itr != link.begin(); --itr) {
                    const Node &check_node = *std::prev(itr);
                    if (check_node.value > b_node.value) {
                        if (check_node.anchor == Center) {
                            if (actor->IsInArea(check_node.actor)) {
                                actor->EnterArea(check_node.actor);
                            }
                        }
                    } else {
                        break;
                    }
                }
                if (itr != itrs[Begin]) {
                    link.splice(itr, link, itrs[Begin]);
                }
            } else if (b_node_value < b_node.value) {
                for (++itr; itr != link.end(); ++itr) {
                    const Node &check_node = *itr;
                    if (check_node.value < b_node.value) {
                        if (check_node.anchor == Center) {
                            actor->LeaveArea(check_node.actor);
                        }
                    } else {
                        break;
                    }
                }
                if (std::prev(itr) != itrs[Begin]) {
                    link.splice(itr, link, itrs[Begin]);
                }
            }
        }

        std::list<Node>::iterator itr = itrs[Center];
        Node &c_node = *itr;
        const float c_node_value = c_node.value;
        c_node.value = GetNodeValue(actor, axis, Center);
        if (c_node_value > c_node.value) {
            for (; itr != link.begin(); --itr) {
                const Node &check_node = *std::prev(itr);
                if (check_node.value > c_node.value) {
                    if (check_node.anchor == Begin) {
                        check_node.actor->LeaveArea(actor);
                    } else if (check_node.anchor == End) {
                        if (check_node.actor->IsInArea(actor)) {
                            check_node.actor->EnterArea(actor);
                        }
                    }
                } else {
                    break;
                }
            }
            if (itr != itrs[Center]) {
                link.splice(itr, link, itrs[Center]);
            }
        } else if (c_node_value < c_node.value) {
            for (++itr; itr != link.end(); ++itr) {
                const Node &check_node = *itr;
                if (check_node.value < c_node.value) {
                    if (check_node.anchor == Begin) {
                        if (check_node.actor->IsInArea(actor)) {
                            check_node.actor->EnterArea(actor);
                        }
                    } else if (check_node.anchor == End) {
                        check_node.actor->LeaveArea(actor);
                    }
                } else {
                    break;
                }
            }
            if (std::prev(itr) != itrs[Center]) {
                link.splice(itr, link, itrs[Center]);
            }
        }

        if (itrs[End]->anchor == End) {
            std::list<Node>::iterator itr = itrs[End];
            Node &e_node = *itr;
            const float e_node_value = e_node.value;
            e_node.value = GetNodeValue(actor, axis, End);
            if (e_node_value > e_node.value) {
                for (; itr != link.begin(); --itr) {
                    const Node &check_node = *std::prev(itr);
                    if (check_node.value > e_node.value) {
                        if (check_node.anchor == Center) {
                            actor->LeaveArea(check_node.actor);
                        }
                    } else {
                        break;
                    }
                }
                if (itr != itrs[End]) {
                    link.splice(itr, link, itrs[End]);
                }
            } else if (e_node_value < e_node.value) {
                for (++itr; itr != link.end(); ++itr) {
                    const Node &check_node = *itr;
                    if (check_node.value < e_node.value) {
                        if (check_node.anchor == Center) {
                            if (actor->IsInArea(check_node.actor)) {
                                actor->EnterArea(check_node.actor);
                            }
                        }
                    } else {
                        break;
                    }
                }
                if (std::prev(itr) != itrs[End]) {
                    link.splice(itr, link, itrs[End]);
                }
            }
        }
    };

    DBGASSERT(stable_status_ == SS_None);
    AutoStableStatus autoStableStatus(this, SS_Write);

    Iterator &itrs = itr->second;
    MoveActorLinkNode(AoiActor::X, x_link_, itrs.xitr);
    MoveActorLinkNode(AoiActor::Z, z_link_, itrs.zitr);
}

void AoiHandler::RemoveActor(AoiActor *actor)
{
    auto itr = itrs_.find(actor);
    if (itr == itrs_.end()) {
        return;
    }

    auto RemoveActorLinkNode =
        [](std::list<Node> &link, std::list<Node>::iterator(&itrs)[N])
    {
        if (itrs[Begin]->anchor == Begin) {
            link.erase(itrs[Begin]);
            link.erase(itrs[End]);
        }
        link.erase(itrs[Center]);
    };

    DBGASSERT(stable_status_ == SS_None);
    AutoStableStatus autoStableStatus(this, SS_Write);

    Iterator &itrs = itr->second;
    RemoveActorLinkNode(x_link_, itrs.xitr);
    RemoveActorLinkNode(z_link_, itrs.zitr);

    itrs_.erase(itr);
}

void AoiHandler::ReloadActorRadius(AoiActor *actor)
{
    auto itr = itrs_.find(actor);
    if (itr == itrs_.end()) {
        return;
    }

    auto Relocate = [actor](AoiActor::Axis axis,
        std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        if (actor->radius() != 0) {
            if (itrs[Begin]->anchor != Begin) {
                itrs[Begin] = link.insert(itrs[Begin],
                    Node{actor, Begin, itrs[Begin]->value});
                itrs[End] = link.insert(std::next(itrs[End]),
                    Node{actor, End, itrs[End]->value});
            }
            do {
                std::list<Node>::iterator itr = itrs[Begin];
                Node &b_node = *itr;
                const float b_node_value = b_node.value;
                b_node.value = GetNodeValue(actor, axis, Begin);
                if (b_node_value > b_node.value) {
                    for (; itr != link.begin() &&
                           std::prev(itr)->value > b_node.value; --itr) {
                        continue;
                    }
                    if (itr != itrs[Begin]) {
                        link.splice(itr, link, itrs[Begin]);
                    }
                } else if (b_node_value < b_node.value) {
                    for (++itr; itr != link.end() &&
                                itr->value < b_node.value; ++itr) {
                        continue;
                    }
                    if (std::prev(itr) != itrs[Begin]) {
                        link.splice(itr, link, itrs[Begin]);
                    }
                }
            } while (0);
            do {
                std::list<Node>::iterator itr = itrs[End];
                Node &e_node = *itr;
                const float e_node_value = e_node.value;
                e_node.value = GetNodeValue(actor, axis, End);
                if (e_node_value > e_node.value) {
                    for (; itr != link.begin() &&
                           std::prev(itr)->value > e_node.value; --itr) {
                        continue;
                    }
                    if (itr != itrs[End]) {
                        link.splice(itr, link, itrs[End]);
                    }
                } else if (e_node_value < e_node.value) {
                    for (++itr; itr != link.end() &&
                                itr->value < e_node.value; ++itr) {
                        continue;
                    }
                    if (std::prev(itr) != itrs[End]) {
                        link.splice(itr, link, itrs[End]);
                    }
                }
            } while (0);
        } else {
            if (itrs[Begin]->anchor == Begin) {
                link.erase(itrs[Begin]);
                link.erase(itrs[End]);
                itrs[Begin] = itrs[End] = itrs[Center];
            }
        }
    };

    do {
        DBGASSERT(stable_status_ == SS_None);
        AutoStableStatus autoStableStatus(this, SS_Write);

        Iterator &itrs = itr->second;
        Relocate(AoiActor::X, x_link_, itrs.xitr);
        Relocate(AoiActor::Z, z_link_, itrs.zitr);
    } while (0);

    ReloadActorSubject(actor);
}

void AoiHandler::ReloadActorObserver(AoiActor *actor) const
{
    auto itr = itrs_.find(actor);
    if (itr == itrs_.end()) {
        return;
    }

    auto Reload = [actor](const std::list<Node> &link,
                          const std::list<Node>::iterator (&itrs)[N])
    {
        std::list<Node>::const_iterator end = itrs[Center];
        std::for_each(link.begin(), end, [actor](const Node &node) {
            if (node.actor != actor && node.anchor == Begin) {
                if (node.actor->IsInArea(actor)) {
                    node.actor->AddSubject(actor);
                }
            }
        });
    };

    DBGASSERT(stable_status_ == SS_None || stable_status_ == SS_Read);
    const AutoStableStatus autoStableStatus(this, SS_Read);

    const Iterator &itrs = itr->second;
    actor->CleanObserver();
    Reload(x_link_, itrs.xitr);
}

void AoiHandler::ReloadActorSubject(AoiActor *actor) const
{
    auto itr = itrs_.find(actor);
    if (itr == itrs_.end()) {
        return;
    }

    auto Reload = [actor](const std::list<Node>::iterator (&itrs)[N]) {
        std::for_each(itrs[Begin], itrs[End], [actor](const Node &node) {
            if (node.actor != actor && node.anchor == Center) {
                if (actor->IsInArea(node.actor)) {
                    actor->AddSubject(node.actor);
                }
            }
        });
    };

    DBGASSERT(stable_status_ == SS_None || stable_status_ == SS_Read);
    const AutoStableStatus autoStableStatus(this, SS_Read);

    const Iterator &itrs = itr->second;
    actor->CleanSubject();
    Reload(itrs.xitr);
}

void AoiHandler::ReloadActorStatus(AoiActor *actor) const
{
    ReloadActorObserver(actor);
    ReloadActorSubject(actor);
}

void AoiHandler::ReloadAllActorSubject() const
{
    for (auto &pair : itrs_) {
        ReloadActorSubject(pair.first);
    }
}

void AoiHandler::CollateAllActorMarker() const
{
    DBGASSERT(stable_status_ == SS_None || stable_status_ == SS_Read);
    const AutoStableStatus autoStableStatus(this, SS_Read);

    for (auto &pair : itrs_) {
        pair.first->CollateMarker();
    }
}

float AoiHandler::GetNodeValue(AoiActor *actor, int axis, int anchor)
{
    switch ((axis << 8) + anchor) {
    case (AoiActor::X << 8) + Begin: return actor->x_left();
    case (AoiActor::X << 8) + Center: return actor->x_center();
    case (AoiActor::X << 8) + End: return actor->x_right();
    case (AoiActor::Z << 8) + Begin: return actor->z_front();
    case (AoiActor::Z << 8) + Center: return actor->z_center();
    case (AoiActor::Z << 8) + End: return actor->z_back();
    default: assert(false && "can't reach here."); return 0;
    }
}
