/// Verifies the VTK writer by parsing its output back (no ParaView needed):
/// header validity, DIMENSIONS/POINT_DATA, and — critically — that each point's
/// value matches the field at the correct (x,y,z) under VTK X-fastest ordering.

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cyberfluids/io/vtk_writer.hpp"
#include "testing.hpp"

namespace {
std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    std::string l;
    while (std::getline(in, l)) lines.push_back(l);
    return lines;
}
int findPrefix(const std::vector<std::string>& lines, const std::string& prefix, int from = 0) {
    for (int i = from; i < static_cast<int>(lines.size()); ++i)
        if (lines[i].rfind(prefix, 0) == 0) return i;
    return -1;
}
}  // namespace

int main() {
    // ---- 2D ----
    {
        const std::int64_t nx = 3, ny = 4;
        cyberfluids::io::VtkStructuredWriter<2> w(nx, ny);
        w.addScalar("s", [](std::array<std::int64_t, 2> c) {
            return static_cast<double>(c[0]) + 1000.0 * c[1];
        });
        w.addVector("v", [](std::array<std::int64_t, 2> c) {
            return std::array<double, 2>{static_cast<double>(c[0]), static_cast<double>(c[1])};
        });
        const std::string path = "test_vtk_2d.vtk";
        w.write(path);

        auto lines = readLines(path);
        CF_CHECK(!lines.empty() && lines[0].rfind("# vtk DataFile Version", 0) == 0);
        CF_CHECK(findPrefix(lines, "ASCII") >= 0);
        CF_CHECK(findPrefix(lines, "DATASET STRUCTURED_POINTS") >= 0);
        {
            int d = findPrefix(lines, "DIMENSIONS");
            std::istringstream ss(lines[d]);
            std::string tok;
            std::int64_t a, b, cc;
            ss >> tok >> a >> b >> cc;
            CF_CHECK(a == nx && b == ny && cc == 1);
        }
        {
            int p = findPrefix(lines, "POINT_DATA");
            std::istringstream ss(lines[p]);
            std::string tok;
            std::int64_t np;
            ss >> tok >> np;
            CF_CHECK(np == nx * ny);
        }
        // Scalar values in VTK order.
        int s0 = findPrefix(lines, "LOOKUP_TABLE") + 1;
        for (std::int64_t k = 0; k < nx * ny; ++k) {
            const std::int64_t x = k % nx, y = (k / nx) % ny;
            CF_CHECK_CLOSE(std::stod(lines[s0 + k]), x + 1000.0 * y, 1e-9);
        }
        // Vector values (3 components, vz==0 in 2D).
        int v0 = findPrefix(lines, "VECTORS v") + 1;
        for (std::int64_t k = 0; k < nx * ny; ++k) {
            const std::int64_t x = k % nx, y = (k / nx) % ny;
            std::istringstream ss(lines[v0 + k]);
            double vx, vy, vz;
            ss >> vx >> vy >> vz;
            CF_CHECK_CLOSE(vx, x, 1e-9);
            CF_CHECK_CLOSE(vy, y, 1e-9);
            CF_CHECK(vz == 0.0);
        }
        std::remove(path.c_str());
    }

    // ---- 3D ----
    {
        const std::int64_t nx = 2, ny = 3, nz = 2;
        cyberfluids::io::VtkStructuredWriter<3> w(nx, ny, nz);
        w.addScalar("s", [](std::array<std::int64_t, 3> c) {
            return c[0] + 1000.0 * c[1] + 1e6 * c[2];
        });
        const std::string path = "test_vtk_3d.vtk";
        w.write(path);
        auto lines = readLines(path);
        int d = findPrefix(lines, "DIMENSIONS");
        std::istringstream ss(lines[d]);
        std::string tok;
        std::int64_t a, b, cc;
        ss >> tok >> a >> b >> cc;
        CF_CHECK(a == nx && b == ny && cc == nz);
        int s0 = findPrefix(lines, "LOOKUP_TABLE") + 1;
        for (std::int64_t k = 0; k < nx * ny * nz; ++k) {
            const std::int64_t x = k % nx, y = (k / nx) % ny, z = k / (nx * ny);
            CF_CHECK_CLOSE(std::stod(lines[s0 + k]), x + 1000.0 * y + 1e6 * z, 1e-3);
        }
        std::remove(path.c_str());
    }

    if (cftest::failures == 0) std::printf("vtk_writer: all checks passed\n");
    return cftest::failures;
}
