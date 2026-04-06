/* Constructors */
template <typename T, std::uint8_t N> ArrayND<T, N>::ArrayND() {
    values.fill(T{});
}

template <typename T, std::uint8_t N> ArrayND<T, N>::ArrayND(const T fillValue) {
    values.fill(fillValue);
}

template <typename T, std::uint8_t N> ArrayND<T, N>::ArrayND(const T x, const T y) {
    values.at(0) = x;
    values.at(1) = y;
}

template <typename T, std::uint8_t N> ArrayND<T, N>::ArrayND(const T x, const T y, const T z) {
    values.at(0) = x;
    values.at(1) = y;
    values.at(2) = z;
}


/* Arthmetic Operators */
template <typename T, std::uint8_t N> ArrayND<T, N> ArrayND<T, N>::operator+(const ArrayND<T, N>& otherArr) const {
    ArrayND<T, N> result = ArrayND<T, N>();

    for(std::uint8_t i = 0; i < N; i++)
        result.values[i] = values[i] + otherArr.values[i];

    return result;
}

template <typename T, std::uint8_t N> ArrayND<T, N> ArrayND<T, N>::operator-(const ArrayND<T, N>& otherArr) const {
    ArrayND<T, N> result = ArrayND<T, N>();

    for(std::uint8_t i = 0; i < N; i++)
        result.values[i] = values[i] - otherArr.values[i];

    return result;
}

template <typename T, std::uint8_t N> ArrayND<T, N> ArrayND<T, N>::operator*(const ArrayND<T, N>& otherArr) const {
    ArrayND<T, N> result = ArrayND<T, N>();

    for(std::uint8_t i = 0; i < N; i++)
        result.values[i] = values[i] * otherArr.values[i];

    return result;
}

template <typename T, std::uint8_t N> ArrayND<T, N> ArrayND<T, N>::operator/(const ArrayND<T, N>& otherArr) const {
    ArrayND<T, N> result = ArrayND<T, N>();

    for(std::uint8_t i = 0; i < N; i++)
        result.values[i] = otherArr.values[i] != 0 ? values[i] / otherArr.values[i] : 0;

    return result;
}

template <typename T, std::uint8_t N> template <typename U> ArrayND<T, N> ArrayND<T, N>::operator/(const U scalar) const {
    ArrayND<T, N> result = ArrayND<T, N>();

    for(std::uint8_t i = 0; i < N; i++)
        result.values[i] = values[i] / scalar;

    return result;
}


/* Assignment Operators */
template <typename T, std::uint8_t N> T ArrayND<T, N>::operator[](uint8_t i) const {
    return values.at(i);
}


template <typename T, std::uint8_t N> T& ArrayND<T, N>::operator[](uint8_t i) {
    return values.at(i);
}


template <typename T, std::uint8_t N> ArrayND<T, N>& ArrayND<T, N>::operator+=(const ArrayND<T, N>& otherArr) {
    for(std::uint8_t i = 0; i < N; i++)
        values[i] += otherArr.values[i];

    return *this;
}

template <typename T, std::uint8_t N> ArrayND<T, N>& ArrayND<T, N>::operator-=(const ArrayND<T, N>& otherArr) {
    for(std::uint8_t i = 0; i < N; i++)
        values[i] -= otherArr.values[i];

    return *this;
}

template <typename T, std::uint8_t N> ArrayND<T, N>& ArrayND<T, N>::operator*=(const ArrayND<T, N>& otherArr) {
    for(std::uint8_t i = 0; i < N; i++)
        values[i] *= otherArr.values[i];

    return *this;
}

template <typename T, std::uint8_t N> ArrayND<T, N>& ArrayND<T, N>::operator/=(const ArrayND<T, N>& otherArr) {
    for(std::uint8_t i = 0; i < N; i++)
        values[i] /= otherArr.values[i] != 0 ? otherArr.values[i] : 1;

    return *this;
}


/* toString() */
template <typename U, std::uint8_t M> std::ostream& operator<<(std::ostream& os, const ArrayND<U, M>& arrNd) {
    os << '(' << arrNd.values[0];

    for(std::uint8_t i = 1; i < M; i++)
        os << ", " << arrNd.values[i];

    return os << ')';
}