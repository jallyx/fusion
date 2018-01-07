#pragma once

#include "TileDefine.h"

class MapTile;

class TileHandler
{
public:
    TileHandler(const TileDefine &tile_define);
    ~TileHandler();

    MapTile *GetTile(size_t x, size_t z) const;
    MapTile *CreateAndGetTile(size_t x, size_t z);
    MapTile *GetTileByCoords(float x, float z) const;
    MapTile *CreateAndGetTileByCoords(float x, float z);

private:
    const TileDefine &tile_define_;
    MapTile ***tiles_;
};
