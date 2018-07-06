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

    void UpdateActive();

    const TileDefine &tile_define() const { return tile_define_; }
    MapTile *&anchor_tile() { return anchor_tile_; }
    TileActor *&anchor_actor() { return anchor_actor_; }

    class AutoTraverseNumber {
    public:
        AutoTraverseNumber(const TileHandler *handler)
        : handler_(handler) { ++handler_->traverse_number_; }
        ~AutoTraverseNumber() { --handler_->traverse_number_; }
    private:
        const TileHandler *handler_;
    };
    size_t traverse_number() const { return traverse_number_; }

private:
    const TileDefine &tile_define_;
    MapTile ***tiles_;
    MapTile *anchor_tile_;
    TileActor *anchor_actor_;

    mutable size_t traverse_number_;

    const static size_t group_count_ = 3;
    int tick_count_;
};
