import CCyberfluids

/// Cyberfluids library version string.
public func version() -> String {
    return String(cString: cf_version())
}

/// A 2D lid-driven cavity (D2Q9, BGK) driven through the C++ core.
public final class Cavity2D {
    private let handle: OpaquePointer
    public let nx: Int
    public let ny: Int

    public init?(nx: Int, ny: Int, omega: Double, lidVelocity: Double) {
        guard let h = cf_cavity2d_create(Int64(nx), Int64(ny), omega, lidVelocity) else {
            return nil
        }
        handle = h
        self.nx = Int(cf_cavity2d_nx(h))
        self.ny = Int(cf_cavity2d_ny(h))
    }

    deinit { cf_cavity2d_destroy(handle) }

    public func run(steps: Int) { cf_cavity2d_run(handle, Int64(steps)) }
    public func step() { cf_cavity2d_step(handle) }

    /// Velocity field, row-major, length nx*ny*2: value(x,y,component) at [(x*ny+y)*2+c].
    public func velocity() -> [Double] {
        var out = [Double](repeating: 0, count: nx * ny * 2)
        out.withUnsafeMutableBufferPointer { cf_cavity2d_velocity(handle, $0.baseAddress) }
        return out
    }

    /// Density field, row-major, length nx*ny: value(x,y) at [x*ny+y].
    public func density() -> [Double] {
        var out = [Double](repeating: 0, count: nx * ny)
        out.withUnsafeMutableBufferPointer { cf_cavity2d_density(handle, $0.baseAddress) }
        return out
    }
}
