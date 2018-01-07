#include "RangeIterator.h"
#include "TileHandler.h"
#include "MapTile.h"

RangeIterator::RangeIterator(const MapTile *tile,
    int types, size_t stop_ring, size_t start_ring)
: handler_(tile->handler())
, origin_(tile->x(), tile->z())
, types_(types)
, stop_ring_(stop_ring)
, tile_path_(start_ring)
, type_(0)
{
}

RangeIterator::RangeIterator(
    const TileHandler *handler, const TilePos &pos,
    int types, size_t stop_ring, size_t start_ring)
: handler_(handler)
, origin_(pos)
, types_(types)
, stop_ring_(stop_ring)
, tile_path_(start_ring)
, type_(0)
{
}
