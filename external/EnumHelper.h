#pragma once

#include <unordered_map>
#include <unordered_set>

namespace Enum {

struct Hash {
    template <typename Key>
    std::size_t operator()(Key key) const {
        return static_cast<std::size_t>(key);
    }
};

template <typename Key, typename T>
using unordered_map = std::unordered_map<Key, T, Hash>;
template <typename Key, typename T>
using unordered_multimap = std::unordered_multimap<Key, T, Hash>;
template <typename Key>
using unordered_set = std::unordered_set<Key, Hash>;
template <typename Key>
using unordered_multiset = std::unordered_multiset<Key, Hash>;

}
