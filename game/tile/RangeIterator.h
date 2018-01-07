#pragma once

#include <stddef.h>
#include "TilePath.h"

#define SCAN_TILE_RING_START 0
#define SCAN_TILE_RING_STOP 3

class TileHandler;
class MapTile;

class TilePos
{
public:
    TilePos(size_t x, size_t z) : x_(x), z_(z) {}
    size_t x() const { return x_; }
    size_t z() const { return z_; }
private:
    const size_t x_, z_;
};

class RangeIterator
{
public:
    RangeIterator(const MapTile *tile, int types,
                  size_t stop_ring = SCAN_TILE_RING_STOP,
                  size_t start_ring = SCAN_TILE_RING_START);
    RangeIterator(const TileHandler *handler, const TilePos &pos, int types,
                  size_t stop_ring = SCAN_TILE_RING_STOP,
                  size_t start_ring = SCAN_TILE_RING_START);

    void Next();

    bool IsEndRing() const;
    bool IsEnd() const;

    static size_t RadiusToStartRing(float radius);
    static size_t RadiusToStopRing(float radius);

private:
    const TileHandler * const handler_;
    const TilePos origin_;
    const int types_;
    const size_t stop_ring_;
    TilePath tile_path_;
    int type_;
};
