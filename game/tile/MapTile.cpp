#include "MapTile.h"
#include "TileHandler.h"
#include "TileActor.h"
#include "Debugger.h"

ThreadSafePool<void, MAX_MAP_TILE_POOL_COUNT> MapTile::s_pool_;

MapTile::MapTile(TileHandler *handler, size_t x, size_t z)
: handler_(handler)
, x_(x)
, z_(z)
{
}

MapTile::~MapTile()
{
}

MapTile::ActorItr MapTile::AddActor(TileActor *actor)
{
    actors_.push_front(actor);
    return actors_.begin();
}

MapTile::ActorItr MapTile::MigrateActor(MapTile *tile, ActorItr itr)
{
    DBGASSERT(handler_->traverse_number() == 0);
    if (handler_->anchor_actor() == *itr) {
        handler_->anchor_actor() = (*itr)->NextActor();
    }
    actors_.splice(actors_.begin(), tile->actors_, itr);
    if (handler_->anchor_tile() != tile && tile->empty()) {
        handler_->TryRecycleTile(tile);
    }
    return actors_.begin();
}

void MapTile::RemoveActor(ActorItr itr)
{
    DBGASSERT(handler_->traverse_number() == 0);
    if (handler_->anchor_actor() == *itr) {
        handler_->anchor_actor() = (*itr)->NextActor();
    }
    actors_.erase(itr);
    if (handler_->anchor_tile() != this && empty()) {
        handler_->TryRecycleTile(this);
    }
}

class MapTile::AutoAnchorTile
{
public:
    AutoAnchorTile(MapTile *tile)
        : anchor_tile_(tile->handler_->anchor_tile())
    {
        anchor_tile_ = tile;
    }
    ~AutoAnchorTile() {
        anchor_tile_ = nullptr;
    }
private:
    MapTile *&anchor_tile_;
};

void MapTile::UpdateActive()
{
    if (!actors_.empty()) {
        AutoAnchorTile autoAnchorTile(this);
        TileActor *&anchor = handler_->anchor_actor();
        TileActor *actor = anchor = actors_.front();
        do {
            actor->UpdateActive();
            actor = actor != anchor ?
                anchor : (anchor = anchor->NextActor());
        } while (actor != nullptr);
    }
    if (empty()) {
        handler_->TryRecycleTile(this);
    }
}
