#!/usr/bin/env bash
# Build and install the CyberdyneCorp scientific suite (NumPP, and SciPP once it
# ships install/export rules) into Cyberfluids/.deps so that
# find_package(NumPP CONFIG) / find_package(SciPP CONFIG) resolve them.
#
# In production these come from Conan/vcpkg packages; this is the
# local-development convenience path.
#
# Usage:
#   scripts/bootstrap_deps.sh [path-to-NumPP-source] [path-to-SciPP-source]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NUMPP_SRC="${1:-$ROOT/../NumPP}"
SCIPP_SRC="${2:-$ROOT/../SciPP}"
PREFIX="$ROOT/.deps"
BUILDROOT="$ROOT/.deps-build"   # kept OUT of PREFIX so build-tree configs never shadow the install

# Portable core count (Linux: nproc, macOS: sysctl).
if command -v nproc >/dev/null 2>&1; then JOBS="$(nproc)"; else JOBS="$(sysctl -n hw.ncpu)"; fi

echo ">> Installing NumPP from $NUMPP_SRC"
if [[ ! -f "$NUMPP_SRC/CMakeLists.txt" ]]; then
  echo "NumPP source not found at '$NUMPP_SRC'. Pass its path as arg 1." >&2
  exit 1
fi
cmake -S "$NUMPP_SRC" -B "$BUILDROOT/numpp" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DNUMPP_BUILD_TESTS=OFF -DNUMPP_BUILD_EXAMPLES=OFF
cmake --build "$BUILDROOT/numpp" --target install -j"$JOBS"
echo ">> NumPP installed to $PREFIX"

# SciPP: only installable once it exposes install(TARGETS/EXPORT) + a config
# package. Attempt it if those rules exist; otherwise skip with a note.
if grep -qE "install\(EXPORT|SciPPConfig" "$SCIPP_SRC/CMakeLists.txt" "$SCIPP_SRC/src/CMakeLists.txt" 2>/dev/null; then
  echo ">> Installing SciPP from $SCIPP_SRC"
  cmake -S "$SCIPP_SRC" -B "$BUILDROOT/scipp" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" -DSCIPP_NUMPP_PREFIX="$PREFIX" \
    -DSCIPP_BUILD_TESTS=OFF
  cmake --build "$BUILDROOT/scipp" --target install -j"$JOBS"
  echo ">> SciPP installed to $PREFIX"
else
  echo ">> SciPP has no install/export rules yet; skipping (not required for the MVP)."
fi
