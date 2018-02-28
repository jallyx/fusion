#pragma once

#include "TileDefine.h"
#include "MapTile.h"

class TileHandler
{
public:
    TileHandler(const TileDefine &tile_define);
    ~TileHandler();

    MapTile *GetTile(size_t x, size_t z) const;
    MapTile *CreateAndGetTile(size_t x, size_t z);
    MapTile *GetTileByCoords(float x, float z) const;
    MapTile *CreateAndGetTileByCoords(float x, float z);

    void TryRecycleTile(MapTile *tile);

    const TileDefine &tile_define() const { return tile_define_; }

private:
    const TileDefine &tile_define_;
    MapTile ***tiles_;
};
