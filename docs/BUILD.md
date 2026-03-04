# OpenErase — Build Guide

## Prerequisites

| Tool | Minimum Version | Install |
|------|----------------|---------|
| C compiler (GCC or Clang) | GCC 7+ / Clang 6+ | See below |
| CMake | 3.10+ | See below |
| Git | Any | `git clone` the repo |

---

## macOS

```bash
# Install Xcode Command Line Tools (includes clang + cmake)
xcode-select --install

# Or install cmake via Homebrew if needed
brew install cmake

# Verify
clang --version
cmake --version
```

---

## Linux (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake

# Verify
gcc --version
cmake --version
```

## Linux (Fedora/RHEL)

```bash
sudo dnf install gcc cmake
```

## Linux (Arch)

```bash
sudo pacman -S gcc cmake
```

---

## Windows

**Option 1: MSYS2 (recommended)**
```bash
# Install MSYS2 from https://msys2.org, then in MSYS2 shell:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
```

**Option 2: Visual Studio**
- Install Visual Studio 2019+ with "Desktop development with C++"
- CMake is included with VS 2019+

**Option 3: WSL2**
Follow the Linux instructions inside WSL2.

> Note: Windows support is planned but not yet implemented. The POSIX engine code (O_DIRECT, /dev/urandom, fsync) will not compile on Windows without the WinAPI layer. See `AGENT.md` Phase 2 for the Windows implementation plan.

---

## Build Steps

```bash
# 1. Clone the repo
git clone https://github.com/RoyRoki/OpenErase.git
cd OpenErase

# 2. Configure (generates build system in ./build/)
cmake -B build

# 3. Build
cmake --build build

# 4. Run
./build/openerase --help
./build/openerase shred /path/to/file
./build/openerase wipe-free /path/to/drive
```

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | (none) | Set to `Release` for optimized build, `Debug` for debug symbols |
| `CMAKE_C_COMPILER` | system default | Override to use a specific compiler |

**Examples:**

```bash
# Debug build with AddressSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-fsanitize=address,undefined"
cmake --build build

# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Use Clang explicitly
cmake -B build -DCMAKE_C_COMPILER=clang
cmake --build build
```

---

## Running Tests

```bash
# Build first (tests are built alongside the main binary)
cmake -B build && cmake --build build

# Run individual test binaries
./build/test_shred
./build/test_wipe

# Run all tests via CTest (with verbose output)
cd build
ctest --output-on-failure

# Run all tests verbose
ctest -V
```

---

## Compiler Warnings

The build enforces strict warnings:
```
-Wall -Wextra -Wpedantic -Werror=vla
```

`-Werror=vla` treats Variable Length Arrays as errors — VLAs can cause stack overflows with large files and are banned in this project. All buffer sizes must be compile-time constants or heap-allocated.

---

## Clean Build

```bash
# Remove the build directory entirely and start fresh
rm -rf build
cmake -B build && cmake --build build
```

---

## Installed Binaries

After building, the following executables are in `./build/`:

| Binary | Description |
|--------|-------------|
| `openerase` | Main CLI tool |
| `test_shred` | Shred engine unit tests |
| `test_wipe` | Wipe engine unit tests |

No install step is required — run directly from the build directory.
