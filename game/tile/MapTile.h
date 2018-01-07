#pragma once

#include <stddef.h>

class TileHandler;

class MapTile
{
public:
    enum Type {Player, Creature,};

    MapTile(TileHandler *handler, size_t x, size_t z);
    ~MapTile();

    TileHandler *handler() const { return handler_; }
    size_t x() const { return x_; }
    size_t z() const { return z_; }

private:
    TileHandler * const handler_;
    size_t const x_, z_;
};
