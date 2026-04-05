#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <cstdint>
#include <iostream>
#include <array>

class Float3D {
    public:
    /* Variables */
    std::array<float, 3> values;

    /* Constructors */
    Float3D();
    Float3D(const float fillValue);
    Float3D(const float x, const float y, const float z);

    /* toString() */
    friend std::ostream& operator<<(std::ostream& os, const Float3D& fl3d);
};

Float3D::Float3D() {
    values.fill(0);
}

Float3D::Float3D(const float fillValue) {
    values.fill(fillValue);
}

Float3D::Float3D(const float x, const float y, const float z) {
    values[0] = x;
    values[1] = y;
    values[2] = z;
}

std::ostream& operator<<(std::ostream& os, const Float3D& fl3d) {
    os << '(' << fl3d.values[0];

    for(uint8_t i = 1; i < 3; i++)
        os << ", " << fl3d.values[i];

    return os << ')';
}

#endif