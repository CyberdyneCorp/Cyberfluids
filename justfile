# Cyberfluids developer tasks — run `just` (or `just --list`) to see everything.
# Requires: cmake >= 3.24, a C++20 compiler, and a NumPP install under .deps
# (see `just bootstrap`). GPU recipes additionally need the matching toolkit:
# CUDA (nvcc + driver), OpenCL (ICD loader + headers), or Metal (Apple + Xcode).
#
# Platform notes baked into the recipes:
#   * Linux/GCC links libstdc++'s parallel backend with `-Wl,--no-as-needed -ltbb`
#     (the CPU par_unseq path needs TBB; --no-as-needed keeps it despite ordering).
#   * macOS/clang libc++ has no parallel-algorithm backend, so no TBB is linked.
#   * CUDA is built for `cuda_arch` (default 90) which embeds SASS + PTX; the PTX
#     lets the driver JIT onto newer GPUs (e.g. Blackwell sm_120) with nvcc < 13.

build_dir  := "build"
deps_prefix := ".deps"
numpp_src  := "../NumPP"
scipp_src  := "../SciPP"
cuda_arch  := "90"

# Show the available recipes (default).
default:
    @just --list

# Build + install the pinned NumPP (and SciPP if it exports install rules) into
# .deps so find_package(NumPP CONFIG) resolves them. Override the source paths:
#   just bootstrap ../NumPP ../SciPP
bootstrap numpp=numpp_src scipp=scipp_src:
    ./scripts/bootstrap_deps.sh {{numpp}} {{scipp}}

# Detect usable GPU backends (CUDA / OpenCL / Metal) and recommend a recipe.
gpu-detect:
    #!/usr/bin/env bash
    set -uo pipefail
    os=$(uname -s); arch=$(uname -m)
    case "$os" in
      MINGW*|MSYS*|CYGWIN*) plat=Windows ;;
      Darwin)               plat=macOS   ;;
      Linux)                plat=Linux   ;;
      *)                    plat="$os"   ;;
    esac
    echo "Host: $plat ($os $arch)"
    echo
    cuda=no; opencl=no; metal=no

    # --- CUDA (NVIDIA) — driver at runtime, nvcc to build ----------------------
    if command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi -L >/dev/null 2>&1; then
      gpu=$(nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | head -1)
      drv=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null | head -1)
      if command -v nvcc >/dev/null 2>&1; then
        nvcc=$(nvcc --version | grep -oE 'release [0-9.]+' | head -1)
      else
        nvcc="nvcc not on PATH — install the CUDA toolkit to build"
      fi
      echo "✅ CUDA   : ${gpu:-NVIDIA GPU} (driver ${drv:-?}, ${nvcc})"
      cuda=yes
    else
      echo "❌ CUDA   : no NVIDIA driver (nvidia-smi) detected"
    fi

    # --- OpenCL — clinfo if present, else probe the ICD loader / framework -----
    if command -v clinfo >/dev/null 2>&1 && [ "$(clinfo -l 2>/dev/null | grep -ciE 'device|platform')" -gt 0 ]; then
      dev=$(clinfo -l 2>/dev/null | grep -iE 'device' | head -1 | sed 's/^[[:space:]]*//')
      echo "✅ OpenCL : ${dev:-device present} (clinfo)"; opencl=yes
    elif [ "$plat" = macOS ] && [ -d /System/Library/Frameworks/OpenCL.framework ]; then
      echo "✅ OpenCL : Apple OpenCL.framework present (install clinfo for details)"; opencl=yes
    elif [ "$plat" = Windows ] && { [ -f /c/Windows/System32/OpenCL.dll ] || reg query "HKLM\\SOFTWARE\\Khronos\\OpenCL\\Vendors" >/dev/null 2>&1; }; then
      echo "✅ OpenCL : Windows OpenCL ICD present (install clinfo to enumerate)"; opencl=yes
    elif ls /etc/OpenCL/vendors/*.icd >/dev/null 2>&1 || ldconfig -p 2>/dev/null | grep -q libOpenCL; then
      echo "✅ OpenCL : ICD loader present (install clinfo to enumerate devices)"; opencl=yes
    else
      echo "❌ OpenCL : no ICD loader / OpenCL runtime detected"
    fi

    # --- Metal (Apple GPU) — Apple platforms only ------------------------------
    if [ "$plat" = macOS ] && [ -d /System/Library/Frameworks/Metal.framework ]; then
      dev=$(system_profiler SPDisplaysDataType 2>/dev/null | grep -iE 'Chipset Model' | head -1 | sed 's/^[[:space:]]*//')
      echo "✅ Metal  : ${dev:-Metal framework present}"; metal=yes
    else
      echo "❌ Metal  : Apple platforms only"
    fi

    echo
    if   [ "$cuda"   = yes ]; then rec="CUDA   →  just cuda    (or: just gpu)"
    elif [ "$metal"  = yes ]; then rec="Metal  →  just metal   (or: just gpu)"
    elif [ "$opencl" = yes ]; then rec="OpenCL →  just opencl  (or: just gpu)"
    else rec="CPU only →  just test   (portable CPU backend; always available)"; fi
    echo "Recommended: $rec"

# Configure a Release build (adds Linux TBB link flags); pass extra cmake flags.
configure *FLAGS:
    #!/usr/bin/env bash
    set -euo pipefail
    LINK=()
    if [ "$(uname -s)" = "Linux" ]; then
      LINK=(-DCMAKE_EXE_LINKER_FLAGS="-Wl,--no-as-needed -ltbb"
            -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--no-as-needed -ltbb")
    fi
    cmake -S . -B {{build_dir}} -DCMAKE_BUILD_TYPE=Release "${LINK[@]}" {{FLAGS}}

# Compile everything already configured into {{build_dir}}.
compile:
    cmake --build {{build_dir}} -j

# Configure (CPU) + compile.
build: configure compile

# Configure + build + run the full CTest suite (CPU backend).
test: build
    ctest --test-dir {{build_dir}} --output-on-failure
alias ctest := test

# Build the CUDA backend and run the CUDA wind-tunnel test (arch override: just cuda native).
cuda arch=cuda_arch:
    @just configure -DCYBERFLUIDS_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES={{arch}}
    just compile
    ctest --test-dir {{build_dir}} -R cuda_wind_tunnel --output-on-failure

# Build with the OpenCL backend and run only the OpenCL wind-tunnel test.
opencl:
    @just configure -DCYBERFLUIDS_OPENCL=ON
    just compile
    ctest --test-dir {{build_dir}} -R opencl_wind_tunnel --output-on-failure

# Build with the Metal backend and run the Metal cavity test (macOS only).
metal:
    @just configure -DCYBERFLUIDS_METAL=ON
    just compile
    ctest --test-dir {{build_dir}} -R metal_cavity --output-on-failure

# Auto-enable every GPU backend this host has (via gpu-detect), then build + test.
gpu:
    #!/usr/bin/env bash
    set -euo pipefail
    FLAGS=()
    if command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi -L >/dev/null 2>&1 && command -v nvcc >/dev/null 2>&1; then
      FLAGS+=(-DCYBERFLUIDS_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES={{cuda_arch}})
    fi
    if ls /etc/OpenCL/vendors/*.icd >/dev/null 2>&1 || ldconfig -p 2>/dev/null | grep -q libOpenCL \
       || [ -d /System/Library/Frameworks/OpenCL.framework ]; then
      FLAGS+=(-DCYBERFLUIDS_OPENCL=ON)
    fi
    if [ "$(uname -s)" = Darwin ] && [ -d /System/Library/Frameworks/Metal.framework ]; then
      FLAGS+=(-DCYBERFLUIDS_METAL=ON)
    fi
    echo ">> configuring with: ${FLAGS[*]:-<none — CPU only>}"
    just configure "${FLAGS[@]}"
    just compile
    ctest --test-dir {{build_dir}} --output-on-failure

# Build + run the throughput benchmark (CPU vs enabled GPU backends), grid+steps as args.
bench nx="160" ny="80" nz="80" steps="500":
    #!/usr/bin/env bash
    set -euo pipefail
    FLAGS=(-DCYBERFLUIDS_BUILD_BENCH=ON)
    if command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi -L >/dev/null 2>&1 && command -v nvcc >/dev/null 2>&1; then
      FLAGS+=(-DCYBERFLUIDS_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES={{cuda_arch}})
    fi
    if ls /etc/OpenCL/vendors/*.icd >/dev/null 2>&1 || ldconfig -p 2>/dev/null | grep -q libOpenCL \
       || [ -d /System/Library/Frameworks/OpenCL.framework ]; then
      FLAGS+=(-DCYBERFLUIDS_OPENCL=ON)
    fi
    just configure "${FLAGS[@]}"
    cmake --build {{build_dir}} --target wind_tunnel_bench -j
    ./{{build_dir}}/bench/wind_tunnel_bench {{nx}} {{ny}} {{nz}} {{steps}}

# Configure + build + test with debug symbols and assertions (separate dir).
debug:
    #!/usr/bin/env bash
    set -euo pipefail
    LINK=()
    [ "$(uname -s)" = Linux ] && LINK=(-DCMAKE_EXE_LINKER_FLAGS="-Wl,--no-as-needed -ltbb")
    cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug "${LINK[@]}"
    cmake --build build-debug -j
    ctest --test-dir build-debug --output-on-failure

# Validate all OpenSpec specs and changes.
spec:
    openspec validate --all --strict

# Full local CI: CPU tests + spec validation.
ci: test spec

# Remove all build directories.
clean:
    rm -rf {{build_dir}} build-debug
