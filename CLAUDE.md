# OpenErase — Claude Project Context

## What This Is

OpenErase is a cross-platform secure data erasure CLI tool written in C11.
It overwrites file data before deletion to prevent forensic recovery.

**Current state:** CLI router complete, all engine files are stubs.

---

## Build & Test Commands

```bash
# Configure + build
cmake -B build && cmake --build build

# Run the binary
./build/openerase --help
./build/openerase shred <file>
./build/openerase wipe-free <path>

# Run tests
./build/test_shred
./build/test_wipe

# Run all tests via CTest
cd build && ctest --output-on-failure
```

---

## File Structure

```
OpenErase/
├── CMakeLists.txt          — Build config (C11, -Wall -Wextra -Wpedantic -Werror=vla)
├── include/
│   └── cli.h               — CLI interface (print_help, parse_and_execute)
├── src/
│   ├── main.c              — Entry point, calls parse_and_execute()
│   ├── cli.c               — CLI router: parses argv, dispatches to engines [COMPLETE]
│   ├── shred.c             — Secure file overwrite + unlink engine [STUB]
│   ├── wipe.c              — Free-space wiping engine [STUB]
│   └── utils.c             — Shared helpers: PRNG, buffer fill, error handling [STUB]
└── test/
    ├── test_shred.c        — Shred engine unit tests [STUB]
    └── test_wipe.c         — Wipe engine unit tests [STUB]
```

---

## Implementation Priority

1. **`utils.c`** first — everything depends on it
   - `fill_random_bytes(buf, len)` — read from `/dev/urandom` or `arc4random`
   - `fill_pattern_bytes(buf, len, byte)` — fill buffer with a constant byte
   - `secure_zero(buf, len)` — zero memory that won't be optimized away
   - `open_direct(path, flags)` — open with `O_DIRECT` / `F_NOCACHE` to bypass cache

2. **`shred.c`** — core secure deletion
   - `secure_shred(const char *path, int passes)` — overwrite N times then unlink
   - Support: DoD 3-pass, Gutmann 35-pass, single random pass

3. **`wipe.c`** — free space wiping
   - `wipe_free_space(const char *path)` — fill free space with temp file, delete

4. **`test/test_shred.c` + `test/test_wipe.c`** — unit tests

---

## C Coding Standards

- **Standard:** C11 strict (`-std=c11 -Wall -Wextra -Wpedantic`)
- **No VLAs** — enforced by `-Werror=vla` in CMakeLists.txt
- **No malloc without free** — every allocation must have a matching free on all paths
- **4-space indentation**, snake_case for all identifiers
- **No global mutable state** — pass context via function parameters
- **Error handling:** all syscalls must check return values; propagate errors up
- **Headers:** every .c file includes its own .h; no cross-module includes except utils.h

---

## Security-Critical Rules

**Never shortcut these:**

- Overwrite passes must use `fsync()` / `fdatasync()` after each pass — no buffered writes
- Use `O_DIRECT` on Linux or `F_NOCACHE` on macOS to bypass kernel page cache
- After overwriting, call `ftruncate()` to zero the file length before `unlink()`
- `secure_zero()` must use `memset_s()` or a `volatile` pointer — compilers will optimize away plain `memset()`
- Never use `rand()` / `srand()` — use `/dev/urandom` or `arc4random_buf()`

---

## Platform Notes

- **POSIX (macOS/Linux):** Primary target. Use `open()`, `write()`, `fsync()`, `unlink()`.
  - macOS: `fcntl(fd, F_NOCACHE, 1)` to bypass cache
  - Linux: `O_DIRECT` flag on `open()` (requires aligned buffers)
- **Windows:** Future target. Will need `CreateFile` with `FILE_FLAG_NO_BUFFERING`, `FlushFileBuffers`, `DeleteFile`.
- Use `#ifdef _WIN32` / `#else` guards for platform divergence — keep in `utils.c`.

---

## Patterns in Existing Code

- `cli.c` dispatches on `argv[1]` with `strcmp()` — new commands follow the same if/else pattern
- Commented-out engine calls in `cli.c` show the expected function signatures:
  - `secure_shred(argv[2])` for shred
  - `wipe_free_space(argv[2])` for wipe-free
- Return 0 on success, 1 on error (consistent across all functions)
