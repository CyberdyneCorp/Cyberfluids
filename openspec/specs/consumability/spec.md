# consumability Specification

## Purpose

Define what it means for this library to be **usable by others** — not just correct, but
adoptable: installable, buildable with minimal setup, packaged for its language ecosystems,
guarded by CI, honest about performance and stability, and governed so outside contributors and
users know how to engage. This capability is a **portable readiness rubric**: the requirements
are written to generalize, so they can serve as an adoption checklist for other libraries. Where a
requirement's mechanics are detailed elsewhere, this spec states the readiness criterion and the
detailed behavior lives in [build-and-packaging](../build-and-packaging/spec.md).

A library is considered "usable by others" only when every requirement below is met.

## Requirements

### Requirement: Consumable installed package
The library SHALL install as a package that downstream projects consume through their build
system's standard discovery mechanism — for a CMake C/C++ library, a `find_package(<Name>)`
CONFIG package exporting namespaced targets — without vendoring sources or hand-editing include
and link paths. Transitive dependencies SHALL be re-resolved by the installed config.

#### Scenario: Downstream project links the installed package
- **WHEN** the library is installed to a prefix and a separate project requests it through the
  standard discovery mechanism (e.g. `find_package`) and links its exported target(s)
- **THEN** the project SHALL configure, build, and run against the installed headers and
  binaries with no manual path wiring and no copied sources

### Requirement: Low-friction source build
Building from a clean checkout SHALL require at most a single configure + build invocation, with
third-party dependencies resolved automatically (from a package manager or a pinned fetch) rather
than a manual multi-repository setup. Absence of an optional component SHALL degrade gracefully,
never breaking the default build.

#### Scenario: Clone-and-build with no manual dependency setup
- **WHEN** a contributor clones the repository and runs the documented configure + build command
  with no prior dependency installation
- **THEN** required dependencies SHALL be resolved automatically at a pinned version and the
  library SHALL build

### Requirement: Language-ecosystem packages build the native library
Every language binding the library ships SHALL provide an ecosystem-native package (e.g. a Python
wheel, a Swift package) that builds and bundles the native library as part of a standard install,
so a user does not perform a separate manual native build. The installed binding SHALL load its
bundled native library, with an environment override available for a prebuilt one.

#### Scenario: Ecosystem install requires no separate native build
- **WHEN** a user installs a binding through its ecosystem tool (e.g. `pip install`) in a clean
  environment with the required toolchain
- **THEN** the native library SHALL be built and bundled automatically and importing/using the
  binding SHALL work without any manually pre-built artifact

### Requirement: Continuous integration gate
Every push to the default branch and every pull request SHALL be built and tested by CI across
the supported platforms, and any project-artifact validation (specs, linters) SHALL run. A pull
request SHALL NOT be mergeable while its build, tests, or validation are failing.

#### Scenario: Failing change is blocked
- **WHEN** a pull request breaks the build, a test, or artifact validation
- **THEN** CI SHALL report a failing required check and the change SHALL be blocked from merge

### Requirement: Documented, reproducible performance references
Any performance claim the project makes SHALL be backed by a runnable benchmark and by recorded
reference results (hardware, configuration, and measured metric) kept in the docs, so claims are
reproducible and comparable rather than anecdotal.

#### Scenario: A performance claim is reproducible
- **WHEN** the docs state a throughput/speed-up figure
- **THEN** a documented command SHALL reproduce that measurement, and the recorded reference
  results SHALL name the hardware and configuration they were taken on

### Requirement: Versioning and stability policy
The project SHALL publish a versioning and API-stability policy: it SHALL follow semantic
versioning, state the current stability expectations (e.g. pre-1.0 may break), carry a
machine-checkable ABI version on any shared library (a SONAME/`SOVERSION`), and maintain a
changelog of notable changes.

#### Scenario: A consumer can reason about compatibility
- **WHEN** a consumer inspects a release
- **THEN** the version SHALL follow semver, the changelog SHALL record what changed, and any
  shipped shared library SHALL carry an ABI version consumers can pin against

### Requirement: Governance and community health files
The repository SHALL provide the community-health files that let outsiders engage safely and
predictably: a `LICENSE`, a `CONTRIBUTING` guide (how to build, test, and submit changes), a
`CODE_OF_CONDUCT`, a `SECURITY` policy with a **private** vulnerability-reporting channel, and
issue/pull-request templates.

#### Scenario: A newcomer knows how to contribute and report safely
- **WHEN** a newcomer opens the repository to file an issue, propose a change, or report a
  vulnerability
- **THEN** they SHALL find the license, a contribution guide, a code of conduct, issue/PR
  templates, and a security policy directing vulnerabilities to a private channel rather than a
  public issue

### Requirement: Onboarding documentation
The project SHALL document how to build, install, consume, and use the library: a getting-started
guide covering the source build, the installed-package path, and each binding; runnable usage
examples; and pointers to the capability/API reference. Documentation SHALL stay truthful to the
shipped code — a change that makes a documented statement false SHALL update the docs in the same
change.

#### Scenario: A new user gets from zero to a running example
- **WHEN** a new user follows the getting-started documentation
- **THEN** they SHALL be able to build or install the library and run a documented example
  without undocumented steps

#### Scenario: Docs do not drift from the code
- **WHEN** a change alters documented behavior (a capability, flag, or interface)
- **THEN** the same change SHALL update the affected documentation and specs so no published
  claim contradicts the code
