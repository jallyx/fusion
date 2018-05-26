#pragma once

#include <list>
#include "ThreadSafePool.h"

#define MAX_MAP_TILE_POOL_COUNT (4096)

class TileHandler;
class TileActor;

class MapTile
{
public:
    MapTile(TileHandler *handler, size_t x, size_t z);
    ~MapTile();

    typedef std::list<TileActor*>::iterator ActorItr;
    ActorItr AddActor(TileActor *actor);
    ActorItr MigrateActor(MapTile *tile, ActorItr itr);
    void RemoveActor(ActorItr itr);

    void UpdateActive();

    ActorItr ActorBegin() { return actors_.begin(); }
    ActorItr ActorEnd() { return actors_.end(); }

    TileHandler *handler() const { return handler_; }
    size_t x() const { return x_; }
    size_t z() const { return z_; }

    bool empty() const { return actors_.empty(); }

    static void *operator new(size_t size) {
        void *tile = s_pool_.Get();
        if (tile == nullptr) {
            tile = ::operator new(size);
        }
        return tile;
    }
    static void operator delete(void *tile) {
        if (!s_pool_.Put(tile)) {
            ::operator delete(tile);
        }
    }

    static void InitPool() {
    }
    static void ClearPool() {
        void *tile = nullptr;
        while ((tile = s_pool_.Get()) != nullptr) {
            ::operator delete(tile);
        }
    }

private:
    class AutoAnchorTile;

    TileHandler * const handler_;
    size_t const x_, z_;

    std::list<TileActor*> actors_;

    static ThreadSafePool<void, MAX_MAP_TILE_POOL_COUNT> s_pool_;
};
