#pragma once

#include "MapTile.h"

class TileActor
{
public:
    TileActor();
    ~TileActor();

    void OnPush(TileHandler *handler, float x, float z);
    void OnMove(float x, float z);
    void OnPopup();

    virtual void UpdateActive(int dt) = 0;

    TileActor *NextActor() const {
        MapTile::ActorItr itr = std::next(itr_);
        return itr != tile_->ActorEnd() ? *itr : nullptr;
    }

    float DistanceForTileActor(float x, float z) const {
        float x_delta = x_ - x;
        float z_delta = z_ - z;
        return x_delta * x_delta + z_delta * z_delta;
    }

    bool IsInTile() const { return tile_ != nullptr; }

private:
    MapTile *tile_;
    MapTile::ActorItr itr_;
    float x_, z_;
};
