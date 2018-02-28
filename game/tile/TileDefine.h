#pragma once

#include <stddef.h>

#define MAP_TILE_SIZE (3.001f)

class TileDefine
{
public:
    TileDefine(float x_min, float x_max, float z_min, float z_max)
        : x_min_(x_min), x_max_(x_max), z_min_(z_min), z_max_(z_max)
        , x_size_(size_t((x_max - x_min) / MAP_TILE_SIZE) + 1)
        , z_size_(size_t((z_max - z_min) / MAP_TILE_SIZE) + 1)
    {}

    size_t CoordsToTileX(float x) const {
        return size_t((x - x_min_) / MAP_TILE_SIZE);
    }
    size_t CoordsToTileZ(float z) const {
        return size_t((z - z_min_) / MAP_TILE_SIZE);
    }

    bool IsValidTile(size_t x, size_t z) const {
        return x < x_size_ && z < z_size_;
    }
    bool IsValidTileByCoords(float x, float z) const {
        return x_min_ <= x && x <= x_max_ && z_min_ <= z && z <= z_max_;
    }

    float x_min() const { return x_min_; }
    float x_max() const { return x_max_; }
    float z_min() const { return z_min_; }
    float z_max() const { return z_max_; }
    size_t x_size() const { return x_size_; }
    size_t z_size() const { return z_size_; }

private:
    const float x_min_, x_max_, z_min_, z_max_;
    const size_t x_size_, z_size_;
};
