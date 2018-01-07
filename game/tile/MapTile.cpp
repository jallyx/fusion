#include "MapTile.h"

MapTile::MapTile(TileHandler *handler, size_t x, size_t z)
: handler_(handler)
, x_(x)
, z_(z)
{
}

MapTile::~MapTile()
{
}
