#pragma once

#include <functional>

class TileHandler;
class TileActor;

class InrangeIterator
{
public:
    InrangeIterator(const TileHandler *handler, float x, float z, float r);

    bool ForeachActor(const std::function<bool(TileActor*)> &func) const;

private:
    const TileHandler * const handler_;
    const float x_, z_, r_;
};
