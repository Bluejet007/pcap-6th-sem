#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <cstdint>
#include <iostream>
#include <array>

template <typename T, std::uint8_t N> class ArrayND {
    public:
    /* Variables */
    std::array<T, N> values;

    /* Constructors */
    ArrayND<T, N>();
    ArrayND<T, N>(const T fillValue);
    ArrayND<T, N>(const T x, const T y);
    ArrayND<T, N>(const T x, const T y, const T z);

    /* Arthmetic Operators */
    ArrayND<T, N> operator+(const ArrayND<T, N>& otherArr) const;
    ArrayND<T, N> operator-(const ArrayND<T, N>& otherArr) const;
    ArrayND<T, N> operator-() const;
    template <typename U, std::uint8_t M> friend ArrayND<U, M> operator*(const ArrayND<U, M> otherArr, const float scalar);
    ArrayND<T, N> operator*(const ArrayND<T, N>& otherArr) const;
    ArrayND<T, N> operator/(const ArrayND<T, N>& otherArr) const;
    template <typename U> ArrayND<T, N> operator/(const U scalar) const;

    /* Assignment Operators */
    T operator[](uint8_t i) const;
    T& operator[](uint8_t i);
    ArrayND<T, N>& operator=(const ArrayND<T, N>& otherArr);
    ArrayND<T, N>& operator+=(const ArrayND<T, N>& otherArr);
    ArrayND<T, N>& operator-=(const ArrayND<T, N>& otherArr);
    ArrayND<T, N>& operator*=(const ArrayND<T, N>& otherArr);
    ArrayND<T, N>& operator*=(const T scalar);
    ArrayND<T, N>& operator/=(const ArrayND<T, N>& otherArr);
    ArrayND<T, N>& operator/=(const T scalar);

    /* toString() */
    template <typename U, std::uint8_t M> friend std::ostream& operator<<(std::ostream& os, const ArrayND<U, M>& arrNd);
};

#include "arrayType.tpp"

#endif