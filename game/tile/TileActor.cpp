#include "TileActor.h"
#include "TileHandler.h"
#include "Exception.h"
#include <assert.h>

TileActor::TileActor()
: tile_(nullptr)
, x_(0)
, z_(0)
{
}

TileActor::~TileActor()
{
    assert(tile_ == nullptr && "please popup before destroy.");
}

void TileActor::OnPush(TileHandler *handler, float x, float z)
{
    assert(tile_ == nullptr && "can't repeat push.");
    tile_ = handler->CreateAndGetTileByCoords(x, z);
    if (tile_ != nullptr) {
        itr_ = tile_->AddActor(this);
        x_ = x, z_ = z;
    } else {
        THROW_EXCEPTION(InternalException());
    }
}

void TileActor::OnMove(float x, float z)
{
    TileHandler *handler = tile_->handler();
    MapTile *tile = handler->CreateAndGetTileByCoords(x, z);
    if (tile != nullptr) {
        if (tile != tile_) {
            itr_ = tile->MigrateActor(tile_, itr_);
            tile_ = tile;
        }
        x_ = x, z_ = z;
    } else {
        THROW_EXCEPTION(InternalException());
    }
}

void TileActor::OnPopup()
{
    tile_->RemoveActor(itr_);
    tile_ = nullptr;
}
