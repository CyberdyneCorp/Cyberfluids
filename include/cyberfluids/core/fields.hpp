#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include <numpp/core/creation.hpp>
#include <numpp/core/dtype.hpp>
#include <numpp/core/ndarray.hpp>

namespace cyberfluids {

/// A scalar macroscopic field (e.g. density) over a 2D/3D grid, backed by a
/// NumPP array in C (row-major) order. See core-data-structures spec.
template <class T, int Dim>
class ScalarField {
    static_assert(std::is_same_v<T, double>, "MVP: double only");
    static_assert(Dim == 2 || Dim == 3, "Dim must be 2 or 3");

public:
    ScalarField(std::int64_t nx, std::int64_t ny) requires(Dim == 2)
        : n_{nx, ny}, data_(numpp::zeros({nx, ny}, numpp::kFloat64)) {}

    ScalarField(std::int64_t nx, std::int64_t ny, std::int64_t nz) requires(Dim == 3)
        : n_{nx, ny, nz}, data_(numpp::zeros({nx, ny, nz}, numpp::kFloat64)) {}

    T& operator()(std::int64_t x, std::int64_t y) requires(Dim == 2) {
        return ptr()[x * n_[1] + y];
    }
    const T& operator()(std::int64_t x, std::int64_t y) const requires(Dim == 2) {
        return ptr()[x * n_[1] + y];
    }
    T& operator()(std::int64_t x, std::int64_t y, std::int64_t z) requires(Dim == 3) {
        return ptr()[(x * n_[1] + y) * n_[2] + z];
    }
    const T& operator()(std::int64_t x, std::int64_t y, std::int64_t z) const
        requires(Dim == 3) {
        return ptr()[(x * n_[1] + y) * n_[2] + z];
    }

    std::int64_t extent(int axis) const { return n_[axis]; }
    numpp::ndarray& array() { return data_; }
    const numpp::ndarray& array() const { return data_; }

private:
    T* ptr() { return data_.typed_data<T>(); }
    const T* ptr() const { return data_.typed_data<T>(); }

    std::array<std::int64_t, Dim> n_;
    numpp::ndarray data_;
};

/// A tensor macroscopic field with `N` components per grid point (e.g. velocity
/// with N=d), backed by a NumPP array of shape {nx, ny(, nz), N} in C order.
template <class T, int N, int Dim>
class TensorField {
    static_assert(std::is_same_v<T, double>, "MVP: double only");
    static_assert(Dim == 2 || Dim == 3, "Dim must be 2 or 3");
    static_assert(N >= 1, "N must be >= 1");

public:
    static constexpr int components = N;

    TensorField(std::int64_t nx, std::int64_t ny) requires(Dim == 2)
        : n_{nx, ny},
          data_(numpp::zeros({nx, ny, static_cast<std::int64_t>(N)}, numpp::kFloat64)) {}

    TensorField(std::int64_t nx, std::int64_t ny, std::int64_t nz) requires(Dim == 3)
        : n_{nx, ny, nz},
          data_(numpp::zeros({nx, ny, nz, static_cast<std::int64_t>(N)}, numpp::kFloat64)) {}

    T& operator()(std::int64_t x, std::int64_t y, int comp) requires(Dim == 2) {
        return ptr()[(x * n_[1] + y) * N + comp];
    }
    const T& operator()(std::int64_t x, std::int64_t y, int comp) const requires(Dim == 2) {
        return ptr()[(x * n_[1] + y) * N + comp];
    }
    T& operator()(std::int64_t x, std::int64_t y, std::int64_t z, int comp)
        requires(Dim == 3) {
        return ptr()[((x * n_[1] + y) * n_[2] + z) * N + comp];
    }
    const T& operator()(std::int64_t x, std::int64_t y, std::int64_t z, int comp) const
        requires(Dim == 3) {
        return ptr()[((x * n_[1] + y) * n_[2] + z) * N + comp];
    }

    std::int64_t extent(int axis) const { return n_[axis]; }
    numpp::ndarray& array() { return data_; }
    const numpp::ndarray& array() const { return data_; }

private:
    T* ptr() { return data_.typed_data<T>(); }
    const T* ptr() const { return data_.typed_data<T>(); }

    std::array<std::int64_t, Dim> n_;
    numpp::ndarray data_;
};

}  // namespace cyberfluids
