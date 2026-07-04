// Palabos oracle generator for the Cyberfluids 3D lid-driven cavity.
// Matches cyberfluids::solver::LidDrivenCavity3D (D3Q19, BGK, lid on the top
// z-face interior driving +x, no-slip on the other walls). Writes steady-state
// centerline velocity profiles to CSV. Physics-level reference (Palabos uses an
// interpolation velocity BC; Cyberfluids uses moving-wall bounce-back).
// Keep parameters in sync with tests/test_oracle_cavity3d.cpp.

#include <fstream>

#include "palabos3D.h"  // all declarations
// Reduced set of template implementations for a BGK multi-block cavity, skipping
// acceleratedLattice/headers3D.hh (parallel exclusive_scan) and
// offLattice/headers3D.hh (a .clone()-on-pointer bug) — neither compiles under
// AppleClang and neither is needed here.
#include <core/headers3D.hh>
// atomicBlock, minus atomicAcceleratedLattice3D.hh (its std::exclusive_scan with
// an execution policy is unimplemented in libc++'s partial PSTL).
#include <atomicBlock/blockLattice3D.hh>
#include <atomicBlock/dataField3D.hh>
#include <atomicBlock/dataProcessingFunctional3D.hh>
#include <atomicBlock/dataProcessorWrapper3D.hh>
#include <atomicBlock/reductiveDataProcessingFunctional3D.hh>
#include <atomicBlock/reductiveDataProcessorWrapper3D.hh>
#include <multiBlock/headers3D.hh>
#include <latticeBoltzmann/headers3D.hh>
#include <basicDynamics/headers3D.hh>
#include <finiteDifference/headers3D.hh>
#include <boundaryCondition/headers3D.hh>
#include <dataProcessors/headers3D.hh>
#include <algorithm/headers3D.hh>
#include <io/headers3D.hh>
#include <libraryInterfaces/headers3D.hh>

using namespace plb;

typedef double T;
#define DESCRIPTOR descriptors::D3Q19Descriptor

static const plint N = 20;
static const T U = 0.05;
static const T OMEGA = (T)1.0 / (T)0.6;  // tau = 0.6 -> nu = 1/30, Re = U*N/nu = 30
static const plint MAX_ITER = 8000;

void cavitySetup(MultiBlockLattice3D<T, DESCRIPTOR> &lattice, plint nx, plint ny, plint nz, T u,
                 OnLatticeBoundaryCondition3D<T, DESCRIPTOR> &bc) {
    bc.setVelocityConditionOnBlockBoundaries(lattice);
    setBoundaryVelocity(lattice, lattice.getBoundingBox(), Array<T, 3>((T)0., (T)0., (T)0.));
    // Moving lid on the top z-face interior (edges stay no-slip), matching Cyberfluids.
    setBoundaryVelocity(lattice, Box3D(1, nx - 2, 1, ny - 2, nz - 1, nz - 1),
                        Array<T, 3>(u, (T)0., (T)0.));
    initializeAtEquilibrium(lattice, lattice.getBoundingBox(), (T)1.,
                            Array<T, 3>((T)0., (T)0., (T)0.));
    lattice.initialize();
}

int main(int argc, char *argv[]) {
    plbInit(&argc, &argv);
    global::directories().setOutputDir("./");

    const plint nx = N, ny = N, nz = N;
    MultiBlockLattice3D<T, DESCRIPTOR> lattice(nx, ny, nz, new BGKdynamics<T, DESCRIPTOR>(OMEGA));
    OnLatticeBoundaryCondition3D<T, DESCRIPTOR> *bc =
        createInterpBoundaryCondition3D<T, DESCRIPTOR>();
    cavitySetup(lattice, nx, ny, nz, U, *bc);

    for (plint iT = 0; iT < MAX_ITER; ++iT) lattice.collideAndStream();

    std::ofstream out("cavity3d_palabos.csv");
    out << "axis,coord,ux,uy,uz\n";
    const plint xc = nx / 2, yc = ny / 2, zc = nz / 2;
    Array<T, 3> vel;
    for (plint z = 0; z < nz; ++z) {
        lattice.get(xc, yc, z).computeVelocity(vel);
        out << "vertical," << z << "," << vel[0] << "," << vel[1] << "," << vel[2] << "\n";
    }
    for (plint x = 0; x < nx; ++x) {
        lattice.get(x, yc, zc).computeVelocity(vel);
        out << "horizontal," << x << "," << vel[0] << "," << vel[1] << "," << vel[2] << "\n";
    }
    out.close();

    delete bc;
    pcout << "wrote cavity3d_palabos.csv (N=" << N << ", U=" << U << ", omega=" << OMEGA
          << ", iters=" << MAX_ITER << ")" << std::endl;
    return 0;
}
