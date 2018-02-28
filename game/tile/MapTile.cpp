#include "MapTile.h"
#include "TileHandler.h"
#include "TileActor.h"
#include "Exception.h"

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
    bool active = *itr == handler_->anchor();
    if (active) {
        handler_->anchor() = (*itr)->NextActor();
    }
    actors_.splice(actors_.begin(), tile->actors_, itr);
    if (!active && tile->empty()) {
        handler_->TryRecycleTile(tile);
    }
    return actors_.begin();
}

void MapTile::RemoveActor(ActorItr itr)
{
    bool active = *itr == handler_->anchor();
    if (active) {
        handler_->anchor() = (*itr)->NextActor();
    }
    actors_.erase(itr);
    if (!active && empty()) {
        handler_->TryRecycleTile(this);
    }
}

void MapTile::UpdateActive(int dt)
{
    if (!actors_.empty()) {
        TileActor *&anchor = handler_->anchor();
        TileActor *actor = anchor = actors_.front();
        do {
            actor->UpdateActive(dt);
            actor = actor != anchor ?
                anchor : (anchor = anchor->NextActor());
        } while (actor != nullptr);
    }
    if (empty()) {
        handler_->TryRecycleTile(this);
    }
}
