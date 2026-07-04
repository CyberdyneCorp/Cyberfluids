// Palabos oracle generator for the Cyberfluids 2D lid-driven cavity.
//
// Runs a lid-driven cavity with the SAME discrete parameters as the Cyberfluids
// solver (D2Q9, BGK, N, omega, lid velocity U, step count) and writes the
// steady-state centerline velocity profiles to CSV. Palabos uses its own
// (interpolation) velocity boundary condition, so this is a physics-level
// reference: the steady-state centerlines must agree with Cyberfluids within a
// documented tolerance (see tests/oracle/README.md).
//
// Keep the parameters here in sync with tests/test_oracle_cavity.cpp.

#include <fstream>

#include "palabos2D.h"
#include "palabos2D.hh"

using namespace plb;

typedef double T;
#define DESCRIPTOR descriptors::D2Q9Descriptor

// Discrete parameters — identical to the Cyberfluids oracle test.
static const plint N = 64;              // grid is N x N
static const T U = 0.05;                // lid velocity (lattice units)
static const T OMEGA = (T)1.0 / (T)0.6; // tau = 0.6  ->  Re = U*N/nu ~= 96
static const plint MAX_ITER = 40000;    // run to steady state

void cavitySetup(MultiBlockLattice2D<T, DESCRIPTOR> &lattice, plint nx, plint ny, T u,
                 OnLatticeBoundaryCondition2D<T, DESCRIPTOR> &bc) {
    bc.setVelocityConditionOnBlockBoundaries(lattice);
    setBoundaryVelocity(lattice, lattice.getBoundingBox(), Array<T, 2>((T)0., (T)0.));
    // Moving lid on the top-row interior (corners stay no-slip), matching Cyberfluids.
    setBoundaryVelocity(lattice, Box2D(1, nx - 2, ny - 1, ny - 1), Array<T, 2>(u, (T)0.));
    initializeAtEquilibrium(lattice, lattice.getBoundingBox(), (T)1., Array<T, 2>((T)0., (T)0.));
    lattice.initialize();
}

int main(int argc, char *argv[]) {
    plbInit(&argc, &argv);
    global::directories().setOutputDir("./");

    const plint nx = N, ny = N;
    MultiBlockLattice2D<T, DESCRIPTOR> lattice(nx, ny, new BGKdynamics<T, DESCRIPTOR>(OMEGA));

    OnLatticeBoundaryCondition2D<T, DESCRIPTOR> *bc =
        createInterpBoundaryCondition2D<T, DESCRIPTOR>();
    cavitySetup(lattice, nx, ny, U, *bc);

    for (plint iT = 0; iT < MAX_ITER; ++iT) lattice.collideAndStream();

    std::ofstream out("cavity2d_palabos.csv");
    out << "axis,coord,ux,uy\n";
    const plint xc = nx / 2, yc = ny / 2;
    Array<T, 2> vel;
    for (plint iY = 0; iY < ny; ++iY) {
        lattice.get(xc, iY).computeVelocity(vel);
        out << "vertical," << iY << "," << vel[0] << "," << vel[1] << "\n";
    }
    for (plint iX = 0; iX < nx; ++iX) {
        lattice.get(iX, yc).computeVelocity(vel);
        out << "horizontal," << iX << "," << vel[0] << "," << vel[1] << "\n";
    }
    out.close();

    delete bc;
    pcout << "wrote cavity2d_palabos.csv (N=" << N << ", U=" << U << ", omega=" << OMEGA
          << ", iters=" << MAX_ITER << ")" << std::endl;
    return 0;
}
