// Swift binding smoke test: build and run a cavity from Swift, read the velocity
// field, and check the lid-driven flow is physical. Exits non-zero on failure.

import Cyberfluids
import Foundation

let n = 32
print("cyberfluids version: \(version())")

guard let cav = Cavity2D(nx: n, ny: n, omega: 1.0, lidVelocity: 0.1) else {
    print("swift smoke FAILED: could not create cavity")
    exit(1)
}
cav.run(steps: 3000)

let u = cav.velocity()
let rho = cav.density()
func ux(_ x: Int, _ y: Int) -> Double { u[(x * n + y) * 2 + 0] }

var maxSpeed = 0.0
for i in stride(from: 0, to: u.count, by: 2) {
    let s = (u[i] * u[i] + u[i + 1] * u[i + 1]).squareRoot()
    if s > maxSpeed { maxSpeed = s }
}
let rhoMin = rho.min() ?? 0
let rhoMax = rho.max() ?? 0
let uxTop = ux(n / 2, n - 2)
let uxBot = ux(n / 2, 1)

guard u.count == n * n * 2, maxSpeed < 0.3, rhoMin > 0.7, rhoMax < 1.3,
      uxTop > 0.0, uxTop > uxBot else {
    print("swift smoke FAILED: maxSpeed=\(maxSpeed) rho=[\(rhoMin),\(rhoMax)] uxTop=\(uxTop) uxBot=\(uxBot)")
    exit(1)
}

print(String(format: "swift smoke OK (max speed %.4f, ux_top %.4f)", maxSpeed, uxTop))
