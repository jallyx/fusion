#include "InrangeIterator.h"
#include "TileHandler.h"
#include "TileActor.h"
#include <algorithm>

class RangePath
{
public:
    RangePath(int r = 0)
        : x_(r), z_(r ? r - 1 : 0), r_(r)
    {}

    void Next() {
        if (x_ == r_ && z_ == r_) {
            x_ = ++r_, z_ = r_ - 1;
        } else if (x_ == r_ && z_ > -r_) {
            --z_;
        } else if (x_ > -r_ && -z_ == r_) {
            --x_;
        } else if (-x_ == r_ && z_ < r_) {
            ++z_;
        } else if (x_ < r_ && z_ == r_) {
            ++x_;
        }
    }

    bool IsEndRing() const {
        return x_ == r_ && z_ == r_;
    }

    int x() const { return x_; }
    int z() const { return z_; }
    int ring() const { return r_; }

private:
    int x_, z_, r_;
};

static inline bool IsInRadiusForRangePath(const RangePath &path, float r)
{
    const int x_delta = std::max(std::abs(path.x()) - 1, 0);
    const int z_delta = std::max(std::abs(path.z()) - 1, 0);
    const int distance = x_delta * x_delta + z_delta * z_delta;
    return distance * MAP_TILE_SIZE * MAP_TILE_SIZE < r * r;
}

InrangeIterator::InrangeIterator(const TileHandler *handler, float x, float z, float r)
: handler_(handler)
, x_(x)
, z_(z)
, r_(r)
{
}

bool InrangeIterator::ForeachActor(const std::function<bool(TileActor*)> &func) const
{
    const TileHandler::AutoTraverseNumber autoTraverseNumber(handler_);
    const TileDefine &tile_define = handler_->tile_define();
    const size_t tile_x = tile_define.CoordsToTileX(x_);
    const size_t tile_z = tile_define.CoordsToTileZ(z_);
    const int tile_r = int(r_ / MAP_TILE_SIZE) + 1;

    auto ForeachTileActor = [this, tile_x, tile_z, &func](const RangePath &path) -> bool {
        MapTile *tile = handler_->GetTile(path.x() + tile_x, path.z() + tile_z);
        if (tile == nullptr || !IsInRadiusForRangePath(path, r_)) {
            return true;
        }
        for (auto itr = tile->ActorBegin(), end = tile->ActorEnd(); itr != end; ++itr) {
            auto actor = *itr;
            if (actor->DistanceForTileActor(x_, z_) > r_ * r_) {
                continue;
            }
            if (!func(actor)) {
                return false;
            }
        }
        return true;
    };

    RangePath path;
    if (!ForeachTileActor(path)) {
        return false;
    }
    while (!path.IsEndRing() || path.ring() < tile_r) {
        path.Next();
        if (!ForeachTileActor(path)) {
            return false;
        }
    }

    return true;
}
