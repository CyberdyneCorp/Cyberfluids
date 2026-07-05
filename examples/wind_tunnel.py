#!/usr/bin/env python3
"""3D wind-tunnel example using the Cyberfluids Python bindings.

Runs external flow past an obstacle and writes a legacy-VTK file you open in
ParaView to see the flow (streamlines / glyphs / a coloured slice).

By default it uses an analytic sphere, which works with any build. Pass
``--stl PATH`` to load a real mesh instead — that path needs a geometry-enabled
build (``-DCYBERFLUIDS_GEOMETRY=ON``, CyberMeshGenerator).

Usage:
    # sphere (no extra build flags needed):
    CYBERFLUIDS_LIBRARY=build/libcyberfluids_c.dylib \
    PYTHONPATH=bindings/python python examples/wind_tunnel.py

    # a mesh (geometry-enabled build):
    ... python examples/wind_tunnel.py --stl path/to/model.stl

Then in ParaView: open wind_tunnel.vtk -> Slice (z mid) coloured by 'speed' ->
Stream Tracer seeded on an upstream line -> Glyph for vectors.
"""
import argparse
import sys

import numpy as np

import cyberfluids as cf


def main() -> int:
    ap = argparse.ArgumentParser(description="Cyberfluids 3D wind tunnel -> VTK")
    ap.add_argument("--stl", help="STL/OBJ mesh to load (needs a geometry-enabled build)")
    ap.add_argument("--resolution", type=int, default=32, help="obstacle cells on longest axis")
    ap.add_argument("--u-in", type=float, default=0.05, help="inflow speed (lattice units)")
    ap.add_argument("--reynolds", type=float, default=100.0, help="Reynolds number")
    ap.add_argument("--steps", type=int, default=20000, help="time steps to run")
    ap.add_argument("--pad-up", type=int, default=16, help="inflow padding (cells)")
    ap.add_argument("--pad-down", type=int, default=48, help="wake padding (cells)")
    ap.add_argument("--pad-lat", type=int, default=12, help="lateral padding (cells)")
    ap.add_argument("--out", default="wind_tunnel.vtk", help="output VTK path")
    args = ap.parse_args()

    print(f"cyberfluids {cf.version()}  (geometry build: {cf.has_geometry()})")

    if args.stl:
        if not cf.has_geometry():
            print("ERROR: --stl needs a geometry-enabled build "
                  "(-DCYBERFLUIDS_GEOMETRY=ON).", file=sys.stderr)
            return 2
        # Characteristic length ~ obstacle resolution; omega from Reynolds.
        omega = cf.WindTunnel.omega_for_reynolds(args.u_in, args.resolution, args.reynolds)
        print(f"Loading + voxelizing {args.stl} (resolution {args.resolution}) ...")
        tunnel = cf.WindTunnel.from_stl(args.stl, resolution=args.resolution,
                                        u_in=args.u_in, omega=omega, pad_up=args.pad_up,
                                        pad_down=args.pad_down, pad_lat=args.pad_lat)
    else:
        # Analytic sphere in a tunnel; diameter is the characteristic length.
        nx, ny, nz, radius = 128, 64, 64, 10.0
        omega = cf.WindTunnel.omega_for_reynolds(args.u_in, 2 * radius, args.reynolds)
        print(f"Sphere obstacle: {nx}x{ny}x{nz} grid, R={radius}, "
              f"Re={args.reynolds}, omega={omega:.3f}")
        tunnel = cf.WindTunnel(nx, ny, nz, omega, args.u_in)
        tunnel.set_sphere(nx * 0.28, ny / 2, nz / 2, radius)

    with tunnel:
        cells = tunnel.nx * tunnel.ny * tunnel.nz
        print(f"Tunnel grid: {tunnel.nx} x {tunnel.ny} x {tunnel.nz}  ({cells:,} cells)")
        print(f"Running {args.steps} steps (progress every ~5%) ...")
        # Run in chunks so progress and stability are visible during a long run.
        chunk = max(1, args.steps // 20)
        done = 0
        while done < args.steps:
            n = min(chunk, args.steps - done)
            tunnel.run(n)
            done += n
            mx = float(np.linalg.norm(tunnel.velocity(), axis=-1).max())
            print(f"  step {done:>6}/{args.steps}   max speed {mx:.4f}", flush=True)
            if not np.isfinite(mx):
                print("ERROR: diverged (non-finite).", file=sys.stderr)
                return 1

        vel = tunnel.velocity()          # (nx, ny, nz, 3)
        speed = np.linalg.norm(vel, axis=-1)
        print(f"speed: min={speed.min():.4f}  max={speed.max():.4f}  "
              f"mean={speed.mean():.4f}")
        if not np.isfinite(speed).all():
            print("ERROR: simulation diverged (non-finite velocity).", file=sys.stderr)
            return 1

        tunnel.write_vtk(args.out)
        print(f"Wrote {args.out} — open in ParaView (Stream Tracer on 'velocity').")

        # Python-side visualization: side (x-z) and top (x-y) mid-slices, each a
        # speed map with in-plane streamlines and the obstacle outlined. Skipped
        # if matplotlib is absent (ParaView + the VTK still cover it).
        png = save_flow_views(vel, tunnel.solid(),
                              args.out.rsplit(".", 1)[0] + "_flow.png")
        if png:
            print(f"Wrote {png}")

    return 0


def save_flow_views(vel: np.ndarray, solid: np.ndarray, path: str):
    """Two-panel figure: side (y-mid, x-z) and top (z-mid, x-y) slices, each the
    speed magnitude with in-plane streamlines and the obstacle masked/outlined.
    Returns the PNG path, or None if matplotlib is unavailable."""
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return None

    nx, ny, nz, _ = vel.shape
    speed = np.linalg.norm(vel, axis=-1)
    yc, zc = ny // 2, nz // 2

    def panel(ax, sp, ua, ub, mask, xlabel, ylabel, title):
        na, nb = sp.shape                       # na along x (flow), nb transverse
        masked = np.ma.array(sp.T, mask=mask.T > 0.5)
        mesh = ax.pcolormesh(np.arange(na), np.arange(nb), masked,
                             cmap="turbo", shading="nearest")
        gx, gy = np.meshgrid(np.arange(na), np.arange(nb))
        ax.streamplot(gx, gy, ua.T, ub.T, color="black", density=1.5,
                      linewidth=0.5, arrowsize=0.6)
        ax.contour(np.arange(na), np.arange(nb), mask.T, levels=[0.5],
                   colors="white", linewidths=1.2)
        ax.set_xlabel(xlabel)
        ax.set_ylabel(ylabel)
        ax.set_title(title)
        ax.set_aspect("equal")
        return mesh

    fig, axes = plt.subplots(2, 1, figsize=(12, 7))
    m0 = panel(axes[0], speed[:, yc, :], vel[:, yc, :, 0], vel[:, yc, :, 2],
               solid[:, yc, :], "x (flow →)", "z (up)",
               "Side view (y mid-plane): speed + streamlines")
    m1 = panel(axes[1], speed[:, :, zc], vel[:, :, zc, 0], vel[:, :, zc, 1],
               solid[:, :, zc], "x (flow →)", "y", "Top view (z mid-plane)")
    fig.colorbar(m0, ax=axes[0], label="speed")
    fig.colorbar(m1, ax=axes[1], label="speed")
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return path


if __name__ == "__main__":
    sys.exit(main())
