#pragma once
#include <unordered_map>
#include <vector>
#include "../core/types.h"

struct GridKey {
    int x, y, z;

    bool operator==(const GridKey& other) const {
        return x==other.x && y==other.y && z==other.z;
    }
};

struct KeyHasher {
    std::size_t operator()(const GridKey& k) const {
        return ((k.x * 73856093) ^ (k.y * 19349663) ^ (k.z * 83492791));
    }
};

class SpatialGrid {
public:
    float cellSize;

    std::unordered_map<GridKey, std::vector<int>, KeyHasher> grid;

    SpatialGrid(float cellSize);

    GridKey getKey(const Vec3& pos);
    void build(const std::vector<Particle>& particles);
    std::vector<int> getNeighbors(const Vec3& pos);
};