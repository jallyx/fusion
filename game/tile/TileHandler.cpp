#include "TileHandler.h"
#include "MapTile.h"
#include "Exception.h"
#include <string.h>

TileHandler::TileHandler(const TileDefine &tile_define)
: tile_define_(tile_define)
, tiles_(nullptr)
{
}

TileHandler::~TileHandler()
{
    if (tiles_ != nullptr) {
        for (size_t x = 0; x < tile_define_.x_size(); ++x) {
            if (tiles_[x] != nullptr) {
                for (size_t z = 0; z < tile_define_.z_size(); ++z) {
                    if (tiles_[x][z] != nullptr) {
                        delete tiles_[x][z];
                    }
                }
                delete [] tiles_[x];
            }
        }
        delete [] tiles_;
    }
}

MapTile *TileHandler::GetTile(size_t x, size_t z) const
{
    if (!tile_define_.IsValidTile(x, z)) return nullptr;
    if (!tiles_ || !tiles_[x]) return nullptr;
    return tiles_[x][z];
}

MapTile *TileHandler::CreateAndGetTile(size_t x, size_t z)
{
    MapTile *tile = GetTile(x, z);
    if (tile == nullptr) {
        if (!tile_define_.IsValidTile(x, z)) {
            THROW_EXCEPTION(InternalException());
        }
        if (tiles_ == nullptr) {
            tiles_ = new MapTile**[tile_define_.x_size()];
            memset(tiles_, 0, sizeof(MapTile**) * tile_define_.x_size());
        }
        if (tiles_[x] == nullptr) {
            tiles_[x] = new MapTile*[tile_define_.z_size()];
            memset(tiles_, 0, sizeof(MapTile*) * tile_define_.z_size());
        }
        tile = tiles_[x][z] = new MapTile(this, x, z);
    }
    return tile;
}

MapTile *TileHandler::GetTileByCoords(float x, float z) const
{
    const size_t tile_x = tile_define_.CoordsToTileX(x);
    const size_t tile_z = tile_define_.CoordsToTileZ(z);
    return GetTile(tile_x, tile_z);
}

MapTile *TileHandler::CreateAndGetTileByCoords(float x, float z)
{
    const size_t tile_x = tile_define_.CoordsToTileX(x);
    const size_t tile_z = tile_define_.CoordsToTileZ(z);
    return CreateAndGetTile(tile_x, tile_z);
}
