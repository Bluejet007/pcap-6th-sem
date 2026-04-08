#include <iostream>
#include "floatGrid.hu"

int main() {
    FloatGrid::initReserves(1024);

    float4 arr1[1024], arr2[1024];
    for(int i = 0; i < 1024; i++) {
        arr1[i] = make_float4(i, i + 1, i + 2, 0);
        arr2[i] = make_float4(2 * i, 3 * i, 2 * i + 1, 0);
    }

    FloatGrid fg1 = FloatGrid(arr1, 1024), fg2 = FloatGrid(arr2, 1024);

    fg1 = fg1 + fg2;

    float4 *data = fg1.getData();
    std::cout << fg1.len << '\n';
    for(int j = 0; j < fg1.len; j++)
        std::cout << data[j].x << ' ' << data[j].y << ' ' << data[j].z << '\n';


    FloatGrid::debugReserves();
    return 0;
}