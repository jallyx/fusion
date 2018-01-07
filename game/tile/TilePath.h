#pragma once

class TilePath
{
public:
    TilePath(int r = 0)
        : x_(r), z_(r?r-1:0), r_(r)
    {}

    void Next() {
        if (x_ == r_ && z_ == r_) {
            x_ = ++r_, z_ = r_-1;
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
