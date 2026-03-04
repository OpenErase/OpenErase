# OpenErase — Agent Brief

## What It Is

OpenErase is a cross-platform CLI tool for **secure data erasure**. It overwrites file contents with patterns before deletion, making forensic recovery impractical. Think `shred(1)` but portable, modern C11, and extensible.

**Two commands:**
- `openerase shred <path>` — overwrite a file N times, then delete it
- `openerase wipe-free <path>` — fill free space on a drive to destroy previously deleted data

---

## Current Status

| Component | File | Status |
|-----------|------|--------|
| CLI router | `src/cli.c` | Complete |
| Entry point | `src/main.c` | Complete |
| CLI header | `include/cli.h` | Complete |
| Utilities | `src/utils.c` | **Stub (empty)** |
| Shred engine | `src/shred.c` | **Stub (empty)** |
| Wipe engine | `src/wipe.c` | **Stub (empty)** |
| Shred tests | `test/test_shred.c` | **Stub (empty)** |
| Wipe tests | `test/test_wipe.c` | **Stub (empty)** |

The CLI compiles and routes correctly. Calling `shred` or `wipe-free` prints the routing message but does nothing — the engine calls are commented out in `cli.c` pending implementation.

---

## Implementation Roadmap

### Phase 1 — Foundation (utils.c + headers)

Create `include/utils.h` and implement `src/utils.c`:

```c
// Cryptographically random bytes — use /dev/urandom or arc4random_buf()
void fill_random_bytes(void *buf, size_t len);

// Fill buffer with a repeating byte pattern (for DoD/Gutmann passes)
void fill_pattern_bytes(void *buf, size_t len, uint8_t byte);

// Zero memory without compiler optimization removal
void secure_zero(void *buf, size_t len);  // use memset_s or volatile ptr

// Open file with cache bypass (O_DIRECT on Linux, F_NOCACHE on macOS)
int open_direct(const char *path, int flags);
```

### Phase 2 — Shred Engine (shred.c + shred.h)

Create `include/shred.h` and implement `src/shred.c`:

```c
typedef enum {
    WIPE_SINGLE = 1,    // 1 random pass (fast, good for SSDs)
    WIPE_DOD    = 3,    // DoD 5220.22-M: 0x00, 0xFF, random
    WIPE_GUTMANN = 35,  // Gutmann 35-pass
} WipeMode;

// Returns 0 on success, -1 on error (sets errno)
int secure_shred(const char *path, WipeMode mode);
```

**Algorithm for `secure_shred`:**
1. `stat()` the file to get size
2. `open()` with `O_RDWR` + cache bypass flags
3. For each pass: `lseek()` to 0, write the full file size with pattern, `fsync()`
4. `ftruncate(fd, 0)` to zero the length
5. `close(fd)`
6. `unlink(path)` to remove the directory entry

### Phase 3 — Wipe Engine (wipe.c + wipe.h)

Create `include/wipe.h` and implement `src/wipe.c`:

```c
// Returns 0 on success, -1 on error
int wipe_free_space(const char *path);
```

**Algorithm for `wipe_free_space`:**
1. Create a temp file in the target directory (e.g., `.openerase_tmp_XXXXXX`)
2. Write random data in chunks until `ENOSPC` (disk full)
3. `fsync()` the temp file
4. `unlink()` the temp file
5. Repeat 1-4 two more times for a 3-pass wipe of free space

---

## Core Algorithms

### DoD 5220.22-M (3-pass)
US Department of Defense standard. Three sequential passes:
- Pass 1: `0x00` (all zeros)
- Pass 2: `0xFF` (all ones)
- Pass 3: random bytes

### Gutmann (35-pass)
Peter Gutmann's 1996 method for overwriting MFM/RLL encoded HDDs.
35 specific byte patterns targeting historical encoding schemes.
Overkill for modern drives but included for compliance requirements.

### Single Random Pass
One pass of cryptographically random bytes via `/dev/urandom`.
Sufficient for modern SSDs due to wear leveling and flash translation layers.
Recommended default for performance-sensitive use cases.

---

## Security Threat Model

**What we protect against:**
- Magnetic remnants on spinning HDDs after single-pass deletion
- Metadata leaks (filename, size, timestamps) via directory entries
- Page cache retaining data after `unlink()` (bypass with `O_DIRECT`/`F_NOCACHE`)

**What we do NOT protect against:**
- SSD wear leveling and over-provisioned blocks (no software can guarantee this)
- Hardware-level NAND flash remapping
- Encrypted drives where key destruction is more effective than overwriting
- Physical disk forensics after drive failure

**Mitigations in design:**
- `fsync()` after every pass — flush to physical media
- Cache bypass flags — prevent OS from serving stale data
- `ftruncate()` before `unlink()` — destroy file metadata
- Cryptographic PRNG only — no `rand()` / `srand()`

---

## Cross-Platform Strategy

**POSIX (macOS + Linux) — Phase 1:**
```c
#ifdef __APPLE__
    fcntl(fd, F_NOCACHE, 1);  // Disable kernel cache
#elif defined(__linux__)
    // O_DIRECT requires 512-byte aligned buffers
    open(path, O_RDWR | O_DIRECT);
#endif
```

**Windows — Phase 2 (future):**
```c
#ifdef _WIN32
    CreateFile(..., FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, ...);
    FlushFileBuffers(handle);
    DeleteFile(path);
#endif
```

All platform divergence lives in `utils.c`. Engines (`shred.c`, `wipe.c`) call utils functions and stay platform-agnostic.

---

## Testing Approach

**Shred tests (`test/test_shred.c`):**
- Create a temp file with known content
- Call `secure_shred()` with each `WipeMode`
- Verify the file no longer exists (`stat()` returns `ENOENT`)
- Verify the original content is not readable before deletion

**Wipe tests (`test/test_wipe.c`):**
- Create a temp directory
- Call `wipe_free_space()` on it
- Verify no temp files remain after completion
- Verify function handles `ENOSPC` gracefully

**Principles:**
- Tests use real temp files in `/tmp` — no mock FS needed for basic coverage
- Each test cleans up after itself regardless of pass/fail
- Tests compile as separate executables (see `CMakeLists.txt`)

---

## Do Not

- Do not use `rand()` / `srand()` anywhere — use `/dev/urandom` or `arc4random_buf()`
- Do not use VLAs — `-Werror=vla` is enforced in the build
- Do not skip `fsync()` after write passes — the whole point is getting data to disk
- Do not ignore `errno` on system call failures
- Do not add `--force` flags that skip passes to "go faster" — that breaks the security guarantee
