#include "AOIManager.h"
#include "AOIEntity.h"
#include "InlineFuncs.h"
#include <assert.h>
#include <algorithm>

AOIManager::AOIManager()
{
}

AOIManager::~AOIManager()
{
}

void AOIManager::AddEntity(AOIEntity *entity)
{
    auto rst = itrs_.insert(std::make_pair(entity, Iterator()));
    if (!rst.second) {
        assert("can't repeat add entity.");
        return;
    }

    auto AddEntityLinkNode = [entity](AOIEntity::Axis axis,
        std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        Node b_node{entity, Begin, GetNodeValue(entity, axis, Begin)};
        auto itr = link.begin();
        for (; itr != link.end(); ++itr) {
            const Node &check_node = *itr;
            if (check_node.value < b_node.value) {
                if (check_node.anchor == Begin) {
                    if (check_node.entity->IsInArea(entity)) {
                        check_node.entity->EnterArea(entity);
                    }
                }
            } else {
                break;
            }
        }
        itrs[Begin] = link.insert(itr, b_node);

        if (entity->radius() != 0) {
            Node c_node{entity, Center, GetNodeValue(entity, axis, Center)};
            for (; itr != link.end(); ++itr) {
                const Node &check_node = *itr;
                if (check_node.value < c_node.value) {
                    if (check_node.anchor == Begin) {
                        if (check_node.entity->IsInArea(entity)) {
                            check_node.entity->EnterArea(entity);
                        }
                    } else if (check_node.anchor == Center) {
                        if (entity->IsInArea(check_node.entity)) {
                            entity->EnterArea(check_node.entity);
                        }
                    }
                } else {
                    break;
                }
            }
            itrs[Center] = link.insert(itr, c_node);

            Node e_node{entity, End, GetNodeValue(entity, axis, End)};
            for (; itr != link.end(); ++itr) {
                const Node &check_node = *itr;
                if (check_node.value < e_node.value) {
                    if (check_node.anchor == Center) {
                        if (entity->IsInArea(check_node.entity)) {
                            entity->EnterArea(check_node.entity);
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

    auto AddEntityLinkNodeFast = [entity](AOIEntity::Axis axis,
        std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        Node b_node{entity, Begin, GetNodeValue(entity, axis, Begin)};
        auto itr = link.begin();
        for (; itr != link.end() && itr->value < b_node.value; ++itr);
        itrs[Begin] = link.insert(itr, b_node);
        if (entity->radius() != 0) {
            Node c_node{entity, Center, GetNodeValue(entity, axis, Center)};
            for (; itr != link.end() && itr->value < c_node.value; ++itr);
            itrs[Center] = link.insert(itr, c_node);
            Node e_node{entity, End, GetNodeValue(entity, axis, End)};
            for (; itr != link.end() && itr->value < e_node.value; ++itr);
            itrs[End] = link.insert(itr, e_node);
        } else {
            itrs[Begin]->anchor = Center;
            itrs[End] = itrs[Center] = itrs[Begin];
        }
    };

    Iterator &itrs = rst.first->second;
    AddEntityLinkNode(AOIEntity::X, x_link_, itrs.xitr);
    AddEntityLinkNodeFast(AOIEntity::Z, z_link_, itrs.zitr);
}

void AOIManager::RemoveEntity(AOIEntity *entity)
{
    auto itr = itrs_.find(entity);
    if (itr == itrs_.end()) {
        return;
    }

    auto RemoveEntityLinkNode =
        [](std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        link.erase(itrs[Begin]);
        if (itrs[Begin]->anchor != Center) {
            link.erase(itrs[Center]);
            link.erase(itrs[End]);
        }
    };

    Iterator &itrs = itr->second;
    RemoveEntityLinkNode(x_link_, itrs.xitr);
    RemoveEntityLinkNode(z_link_, itrs.zitr);

    itrs_.erase(itr);
}

void AOIManager::MoveEntity(AOIEntity *entity)
{
    auto itr = itrs_.find(entity);
    if (itr == itrs_.end()) {
        return;
    }

    auto MoveEntityLinkNode = [entity](AOIEntity::Axis axis,
        std::list<Node> &link, std::list<Node>::iterator (&itrs)[N])
    {
        if (itrs[Begin]->anchor != Center) {
            itrs[Begin]->value = GetNodeValue(entity, axis, Begin);
            const Node &b_node = *itrs[Begin];
            for (auto itr = itrs[Begin]; itr != link.begin(); --itr) {
                const Node &check_node = *Advance(itr, -1);
                if (check_node.value > b_node.value) {
                    if (check_node.anchor == Center) {
                        if (entity->IsInArea(check_node.entity)) {
                            entity->EnterArea(check_node.entity);
                        }
                    }
                } else {
                    if (itr != itrs[Begin]) {
                        link.splice(itr, link, itrs[Begin]);
                    }
                    break;
                }
            }
            for (auto itr = Advance(itrs[Begin], 1); itr != link.end(); ++itr) {
                const Node &check_node = *itr;
                if (check_node.value < b_node.value) {
                    if (check_node.anchor == Center) {
                        entity->LeaveArea(check_node.entity);
                    }
                } else {
                    if (Advance(itr, -1) != itrs[Begin]) {
                        link.splice(itr, link, itrs[Begin]);
                    }
                    break;
                }
            }
        }

        itrs[Center]->value = GetNodeValue(entity, axis, Center);
        const Node &c_node = *itrs[Center];
        for (auto itr = itrs[Center]; itr != link.begin(); --itr) {
            const Node &check_node = *Advance(itr, -1);
            if (check_node.value > c_node.value) {
                if (check_node.anchor == Begin) {
                    check_node.entity->LeaveArea(entity);
                } else if (check_node.anchor == End) {
                    if (check_node.entity->IsInArea(entity)) {
                        check_node.entity->EnterArea(entity);
                    }
                }
            } else {
                if (itr != itrs[Center]) {
                    link.splice(itr, link, itrs[Center]);
                }
                break;
            }
        }
        for (auto itr = Advance(itrs[Center], 1); itr != link.end(); ++itr) {
            const Node &check_node = *itr;
            if (check_node.value < c_node.value) {
                if (check_node.anchor == Begin) {
                    if (check_node.entity->IsInArea(entity)) {
                        check_node.entity->EnterArea(entity);
                    }
                } else if (check_node.anchor == End) {
                    check_node.entity->LeaveArea(entity);
                }
            } else {
                if (Advance(itr, -1) != itrs[Center]) {
                    link.splice(itr, link, itrs[Center]);
                }
                break;
            }
        }

        if (itrs[End]->anchor != Center) {
            itrs[End]->value = GetNodeValue(entity, axis, End);
            const Node &e_node = *itrs[End];
            for (auto itr = itrs[End]; itr != link.begin(); --itr) {
                const Node &check_node = *Advance(itr, -1);
                if (check_node.value > e_node.value) {
                    if (check_node.anchor == Center) {
                        entity->LeaveArea(check_node.entity);
                    }
                } else {
                    if (itr != itrs[End]) {
                        link.splice(itr, link, itrs[End]);
                    }
                    break;
                }
            }
            for (auto itr = Advance(itrs[End], 1); itr != link.end(); ++itr) {
                const Node &check_node = *itr;
                if (check_node.value < e_node.value) {
                    if (check_node.anchor == Center) {
                        if (entity->IsInArea(check_node.entity)) {
                            entity->EnterArea(check_node.entity);
                        }
                    }
                } else {
                    if (Advance(itr, -1) != itrs[End]) {
                        link.splice(itr, link, itrs[End]);
                    }
                    break;
                }
            }
        }
    };

    Iterator &itrs = itr->second;
    MoveEntityLinkNode(AOIEntity::X, x_link_, itrs.xitr);
    MoveEntityLinkNode(AOIEntity::Z, z_link_, itrs.zitr);
}

void AOIManager::ReloadEntityObserver(AOIEntity *entity) const
{
    auto itr = itrs_.find(entity);
    if (itr == itrs_.end()) {
        return;
    }

    auto Reload = [entity](const std::list<Node> &link,
                           const std::list<Node>::iterator (&itrs)[N])
    {
        std::list<Node>::const_iterator end = itrs[Center];
        std::for_each(link.begin(), end, [entity](const Node &node) {
            if (node.entity != entity && node.anchor == Begin) {
                if (node.entity->IsInArea(entity)) {
                    node.entity->AddSubject(entity);
                }
            }
        });
    };

    const Iterator &itrs = itr->second;
    entity->CleanObserver();
    Reload(x_link_, itrs.xitr);
}

void AOIManager::ReloadEntitySubject(AOIEntity *entity) const
{
    auto itr = itrs_.find(entity);
    if (itr == itrs_.end()) {
        return;
    }

    auto Reload = [entity](const std::list<Node>::iterator (&itrs)[N]) {
        std::for_each(itrs[Begin], itrs[End], [entity](const Node &node) {
            if (node.entity != entity && node.anchor == Center) {
                if (entity->IsInArea(node.entity)) {
                    entity->AddSubject(node.entity);
                }
            }
        });
    };

    const Iterator &itrs = itr->second;
    entity->CleanSubject();
    Reload(itrs.xitr);
}

void AOIManager::ReloadAllEntitySubject() const
{
    for (auto &pair : itrs_) {
        ReloadEntitySubject(pair.first);
    }
}

float AOIManager::GetNodeValue(AOIEntity *entity, int axis, int anchor)
{
    switch ((axis << 8) + anchor) {
    case (AOIEntity::X << 8) + Begin: return entity->x_left();
    case (AOIEntity::X << 8) + Center: return entity->x_center();
    case (AOIEntity::X << 8) + End: return entity->x_right();
    case (AOIEntity::Z << 8) + Begin: return entity->z_front();
    case (AOIEntity::Z << 8) + Center: return entity->z_center();
    case (AOIEntity::Z << 8) + End: return entity->z_back();
    default: assert("can't reach here."); return 0;
    }
}
