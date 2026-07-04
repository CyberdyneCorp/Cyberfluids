#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>

namespace cyberfluids::io {

/// Writes macroscopic fields to a legacy VTK STRUCTURED_POINTS file (ASCII),
/// openable directly in ParaView with no external dependency. Register named
/// scalar and vector fields as accessors over integer coordinates; `write()`
/// emits points in VTK order (X fastest, then Y, then Z). In 2D the third grid
/// dimension is 1 and vectors are written with a zero third component.
/// See openspec/specs/geometry-and-io/spec.md.
template <int Dim>
class VtkStructuredWriter {
    static_assert(Dim == 2 || Dim == 3, "Dim must be 2 or 3");

public:
    using Coord = std::array<std::int64_t, Dim>;
    using ScalarFn = std::function<double(Coord)>;
    using VectorFn = std::function<std::array<double, Dim>(Coord)>;

    VtkStructuredWriter(std::int64_t nx, std::int64_t ny) requires(Dim == 2) : n_{nx, ny} {}
    VtkStructuredWriter(std::int64_t nx, std::int64_t ny, std::int64_t nz) requires(Dim == 3)
        : n_{nx, ny, nz} {}

    void addScalar(std::string name, ScalarFn fn) {
        scalars_.push_back({std::move(name), std::move(fn)});
    }
    void addVector(std::string name, VectorFn fn) {
        vectors_.push_back({std::move(name), std::move(fn)});
    }

    void write(const std::string& path,
               const std::string& title = "Cyberfluids field export") const {
        std::ofstream out(path);
        out << std::setprecision(15);
        const std::int64_t nx = n_[0], ny = n_[1];
        const std::int64_t nz = (Dim == 3) ? n_[2] : 1;

        out << "# vtk DataFile Version 3.0\n"
            << title << "\nASCII\nDATASET STRUCTURED_POINTS\n"
            << "DIMENSIONS " << nx << " " << ny << " " << nz << "\n"
            << "ORIGIN 0 0 0\nSPACING 1 1 1\n"
            << "POINT_DATA " << (nx * ny * nz) << "\n";

        for (const auto& s : scalars_) {
            out << "SCALARS " << s.name << " double 1\nLOOKUP_TABLE default\n";
            forEachPoint([&](Coord c) { out << s.fn(c) << "\n"; });
        }
        for (const auto& v : vectors_) {
            out << "VECTORS " << v.name << " double\n";
            forEachPoint([&](Coord c) {
                const auto u = v.fn(c);
                if constexpr (Dim == 3)
                    out << u[0] << " " << u[1] << " " << u[2] << "\n";
                else
                    out << u[0] << " " << u[1] << " 0\n";
            });
        }
    }

private:
    template <class Emit>
    void forEachPoint(Emit&& emit) const {
        const std::int64_t nx = n_[0], ny = n_[1];
        const std::int64_t nz = (Dim == 3) ? n_[2] : 1;
        for (std::int64_t z = 0; z < nz; ++z)
            for (std::int64_t y = 0; y < ny; ++y)
                for (std::int64_t x = 0; x < nx; ++x) {
                    Coord c{};
                    c[0] = x;
                    c[1] = y;
                    if constexpr (Dim == 3) c[2] = z;
                    emit(c);
                }
    }

    struct Scalar {
        std::string name;
        ScalarFn fn;
    };
    struct Vector {
        std::string name;
        VectorFn fn;
    };

    std::array<std::int64_t, Dim> n_;
    std::vector<Scalar> scalars_;
    std::vector<Vector> vectors_;
};

}  // namespace cyberfluids::io
