import CCyberfluids

/// A 3D wind tunnel: external flow past an obstacle (D3Q19), driven through the
/// C++ core. The obstacle is an analytic sphere (`setSphere`, always available)
/// or a voxelized STL/OBJ mesh (`fromSTL`, needs a geometry-enabled build). Write
/// the result to VTK and open it in ParaView for streamlines.
public final class WindTunnel {
    private let handle: OpaquePointer
    public let nx: Int
    public let ny: Int
    public let nz: Int

    private init(_ h: OpaquePointer) {
        handle = h
        nx = Int(cf_wind_tunnel_nx(h))
        ny = Int(cf_wind_tunnel_ny(h))
        nz = Int(cf_wind_tunnel_nz(h))
    }

    /// An empty tunnel; add an obstacle with `setSphere`.
    public convenience init?(nx: Int, ny: Int, nz: Int, omega: Double, inflow: Double) {
        guard let h = cf_wind_tunnel_create(Int64(nx), Int64(ny), Int64(nz), omega, inflow)
        else { return nil }
        self.init(h)
    }

    /// Build a tunnel sized around a voxelized STL/OBJ obstacle. Returns nil when
    /// the library lacks geometry support (see `geometryAvailable`) or on read
    /// failure. `resolution` = obstacle cells along its longest axis.
    public static func fromSTL(_ path: String, resolution: Int = 32, inflow: Double = 0.05,
                               omega: Double = 1.8, padUp: Int = 16, padDown: Int = 48,
                               padLat: Int = 12) -> WindTunnel? {
        guard let h = cf_wind_tunnel_create_from_stl(path, Int32(resolution), Int32(padUp),
                                                     Int32(padDown), Int32(padLat), omega, inflow)
        else { return nil }
        return WindTunnel(h)
    }

    /// True if the library was built with STL/OBJ geometry support.
    public static func geometryAvailable() -> Bool { cf_geometry_available() == 1 }

    /// Relaxation rate for a target Reynolds number Re = inflow * lChar / nu.
    public static func omegaForReynolds(inflow: Double, lChar: Double, re: Double) -> Double {
        let nu = inflow * lChar / re
        return 1.0 / (3.0 * nu + 0.5)
    }

    deinit { cf_wind_tunnel_destroy(handle) }

    /// Set a solid sphere obstacle (centre and radius in lattice cells).
    public func setSphere(cx: Double, cy: Double, cz: Double, radius: Double) {
        cf_wind_tunnel_set_sphere(handle, cx, cy, cz, radius)
    }

    public func run(steps: Int) { cf_wind_tunnel_run(handle, Int64(steps)) }

    /// Velocity field, row-major, length nx*ny*nz*3: value(x,y,z,c) at [((x*ny+y)*nz+z)*3+c].
    public func velocity() -> [Double] {
        var out = [Double](repeating: 0, count: nx * ny * nz * 3)
        out.withUnsafeMutableBufferPointer { cf_wind_tunnel_velocity(handle, $0.baseAddress) }
        return out
    }

    /// Solid-fraction field, row-major, length nx*ny*nz: value(x,y,z) at [(x*ny+y)*nz+z].
    public func solid() -> [Double] {
        var out = [Double](repeating: 0, count: nx * ny * nz)
        out.withUnsafeMutableBufferPointer { cf_wind_tunnel_solid(handle, $0.baseAddress) }
        return out
    }

    /// Write a legacy-VTK STRUCTURED_POINTS file (velocity, speed, solid) for ParaView.
    public func writeVTK(_ path: String) { cf_wind_tunnel_write_vtk(handle, path) }
}
