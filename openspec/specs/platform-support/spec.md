# platform-support

## Purpose

Cyberfluids SHALL compile and run natively across desktop/server and mobile targets from a
single C++20 codebase, so the same simulation engine runs on supercomputers, desktops, and
phones/tablets.

## Requirements

### Requirement: Desktop and server targets
The library SHALL build and run on x86-64 (AVX-512-capable) and ARM64 desktop/server
platforms using GCC, Clang, or AppleClang.

#### Scenario: Desktop build
- **WHEN** the library is built on a supported desktop/server platform with a C++20 compiler
- **THEN** the CPU backend SHALL build and pass its test suite

### Requirement: iOS / iPadOS support
The library SHALL be buildable for iOS/iPadOS on Apple Silicon (M-series) and A-series chips
via native Clang, with no reliance on desktop-only APIs in the core.

#### Scenario: Apple mobile build
- **WHEN** the core library and Swift binding are cross-compiled for an iOS/iPadOS target
- **THEN** they SHALL compile and a sample simulation SHALL run on-device

### Requirement: Android NDK support
The library SHALL be cross-compilable for Android ARM targets via the Android NDK.

#### Scenario: Android build
- **WHEN** the core library is built with the Android NDK for an ARM ABI
- **THEN** it SHALL compile and a sample simulation SHALL run on-device

### Requirement: Core free of platform-specific assumptions
The core (physics, data structures, CPU backend) SHALL NOT hard-depend on any single OS or
desktop-only API; platform-specific code SHALL be confined to backend/binding layers.

#### Scenario: Portable core
- **WHEN** the core is compiled for a mobile target
- **THEN** it SHALL require no changes to physics or data-structure code
