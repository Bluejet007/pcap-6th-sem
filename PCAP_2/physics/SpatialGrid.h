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

// unified cell
struct GridCell {
    std::vector<int> particleIndices;
    std::vector<int> triangleIndices;
};

class SpatialGrid {
public:
    float cellSize;

    std::unordered_map<GridKey, GridCell, KeyHasher> grid;

    SpatialGrid(float cellSize);

    GridKey getKey(const Vec3& pos);

    void clear();

    void buildParticles(const std::vector<Particle>& particles);

    void insertTriangle(int idx, const Vec3& pos);

    std::vector<int> getNearbyParticles(const Vec3& pos);
    std::vector<int> getNearbyTriangles(const Vec3& pos);
};