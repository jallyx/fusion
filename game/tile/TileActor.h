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

    float DistanceForTileActor(float x, float z) const {
        float x_delta = x_ - x;
        float z_delta = z_ - z;
        return x_delta * x_delta + z_delta * z_delta;
    }

    bool IsInTile() const { return tile_ != NULL; }

private:
    MapTile *tile_;
    MapTile::ActorItr itr_;
    float x_, z_;
};
