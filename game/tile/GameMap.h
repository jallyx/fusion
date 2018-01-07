#pragma once

#include "TileDefine.h"

class GameMap
{
public:
    GameMap();
    ~GameMap();

    const TileDefine &tile_define() const { return tile_define_; }

private:
    TileDefine tile_define_;
};
