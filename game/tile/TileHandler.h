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

    void UpdateActive(int dt);

    const TileDefine &tile_define() const { return tile_define_; }
    TileActor *&anchor() { return anchor_; }

private:
    const TileDefine &tile_define_;
    MapTile ***tiles_;
    TileActor *anchor_;

    const static size_t group_count_ = 3;
    int group_time_[group_count_];
    int tick_count_;
};
