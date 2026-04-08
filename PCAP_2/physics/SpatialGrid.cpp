#include "SpatialGrid.h"
#include <cmath>

SpatialGrid::SpatialGrid(float cell) : cellSize(cell) {}

GridKey SpatialGrid::getKey(const Vec3& pos) {
    return {
        int(floor(pos.x / cellSize)),
        int(floor(pos.y / cellSize)),
        int(floor(pos.z / cellSize))
    };
}

void SpatialGrid::build(const std::vector<Particle>& particles) {
    grid.clear();

    for (int i = 0; i < particles.size(); i++) {
        GridKey key = getKey(particles[i].pos);
        grid[key].push_back(i);
    }
}

std::vector<int> SpatialGrid::getNeighbors(const Vec3& pos) {
    std::vector<int> result;
    GridKey base = getKey(pos);

    for (int dx=-1; dx<=1; dx++)
    for (int dy=-1; dy<=1; dy++)
    for (int dz=-1; dz<=1; dz++) {

        GridKey k = {base.x+dx, base.y+dy, base.z+dz};

        if (grid.find(k) != grid.end()) {
            result.insert(result.end(),
                          grid[k].begin(),
                          grid[k].end());
        }
    }

    return result;
}