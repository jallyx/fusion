#include "MapTile.h"
#include "TileHandler.h"

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
    actors_.splice(actors_.begin(), tile->actors_, itr);
    handler_->TryRecycleTile(tile);
    return actors_.begin();
}

void MapTile::RemoveActor(ActorItr itr)
{
    actors_.erase(itr);
    handler_->TryRecycleTile(this);
}
