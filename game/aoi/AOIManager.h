#pragma once

#include <list>
#include <unordered_map>

class AOIEntity;

class AOIManager
{
public:
    AOIManager();
    ~AOIManager();

    void AddEntity(AOIEntity *entity);
    void RemoveEntity(AOIEntity *entity);
    void MoveEntity(AOIEntity *entity);

    void ReloadEntityObserver(AOIEntity *entity) const;
    void ReloadEntitySubject(AOIEntity *entity) const;
    void ReloadAllEntitySubject() const;

private:
    static float GetNodeValue(AOIEntity *entity, int axis, int anchor);

    enum Anchor {Begin, Center, End, N};
    struct Node {
        AOIEntity *entity;
        Anchor anchor;
        float value;
    };
    struct Iterator {
        std::list<Node>::iterator xitr[N];
        std::list<Node>::iterator zitr[N];
    };

    std::list<Node> x_link_;
    std::list<Node> z_link_;
    std::unordered_map<AOIEntity*, Iterator> itrs_;
};
