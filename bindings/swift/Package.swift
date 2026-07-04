// swift-tools-version:5.9
import PackageDescription

// Swift binding for Cyberfluids. Wraps the C ABI (libcyberfluids_c) via a
// system-library module. Build/run needs the C header include path and the
// library search + rpath (see bindings/swift/README.md):
//
//   swift run --package-path bindings/swift \
//     -Xcc -I<repo>/include \
//     -Xlinker -L<repo>/build \
//     -Xlinker -rpath -Xlinker <repo>/build \
//     -Xlinker -rpath -Xlinker <repo>/.deps/lib \
//     cavity-demo
let package = Package(
    name: "Cyberfluids",
    platforms: [.macOS(.v13)],
    products: [
        .library(name: "Cyberfluids", targets: ["Cyberfluids"]),
        .executable(name: "cavity-demo", targets: ["CavityDemo"]),
    ],
    targets: [
        .systemLibrary(name: "CCyberfluids", path: "Sources/CCyberfluids"),
        .target(name: "Cyberfluids", dependencies: ["CCyberfluids"]),
        .executableTarget(name: "CavityDemo", dependencies: ["Cyberfluids"]),
    ]
)
