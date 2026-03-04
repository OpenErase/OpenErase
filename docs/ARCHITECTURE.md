# OpenErase — Architecture

## System Diagram

```
┌─────────────────────────────────────────────────────┐
│                    User / Shell                      │
│          openerase shred <path>                      │
│          openerase wipe-free <path>                  │
└────────────────────┬────────────────────────────────┘
                     │ argc / argv
                     ▼
┌─────────────────────────────────────────────────────┐
│                  main.c (entry)                      │
│          parse_and_execute(argc, argv)               │
└────────────────────┬────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────┐
│                  cli.c (router)                      │
│  argv[1] == "shred"    → secure_shred(path, mode)   │
│  argv[1] == "wipe-free"→ wipe_free_space(path)      │
│  argv[1] == "--help"   → print_help()               │
└──────────┬──────────────────────┬───────────────────┘
           │                      │
           ▼                      ▼
┌──────────────────┐   ┌──────────────────────────────┐
│   shred.c        │   │   wipe.c                     │
│  secure_shred()  │   │  wipe_free_space()           │
│  (overwrite +    │   │  (fill free space +          │
│   unlink file)   │   │   delete temp file)          │
└────────┬─────────┘   └──────────────┬───────────────┘
         │                            │
         └──────────┬─────────────────┘
                    ▼
         ┌────────────────────┐
         │   utils.c          │
         │  fill_random_bytes │
         │  fill_pattern_bytes│
         │  secure_zero       │
         │  open_direct       │
         └────────────────────┘
                    │
                    ▼
         ┌────────────────────┐
         │  OS / Filesystem   │
         │  /dev/urandom      │
         │  open/write/fsync  │
         │  unlink/ftruncate  │
         └────────────────────┘
```

---

## Module Responsibilities

### `main.c`
Single responsibility: call `parse_and_execute()` and return its exit code.
No logic lives here.

### `cli.c` + `include/cli.h`
- Parse `argc`/`argv`
- Validate argument count per command
- Dispatch to the correct engine function
- Print help text
- Return exit codes (0 = success, 1 = error)

**Status:** Complete. Engine calls are commented out pending implementation.

### `shred.c` + `include/shred.h` (stub)
- Accept a file path and a `WipeMode` enum
- Overwrite the file N times using the selected algorithm
- Flush each pass to physical media with `fsync()`
- Truncate then unlink the file

### `wipe.c` + `include/wipe.h` (stub)
- Accept a directory/drive path
- Create a temporary file and fill it until the disk is full
- Flush and delete the temp file
- Repeat for multi-pass free space wiping

### `utils.c` + `include/utils.h` (stub)
Shared low-level helpers used by both engines:
- Cryptographic random byte generation
- Pattern buffer fills (for DoD/Gutmann passes)
- Cache-bypassing file open
- Compiler-safe memory zeroing

---

## Data Flow: `shred` Command

```
openerase shred /path/to/secret.txt
          │
          ▼
parse_and_execute(argc=3, argv=["openerase","shred","/path/to/secret.txt"])
          │
          ▼  strcmp(argv[1], "shred") == 0
cli.c → secure_shred("/path/to/secret.txt", WIPE_DOD)
          │
          ├─ stat(path) → get file size (N bytes)
          ├─ open_direct(path, O_RDWR) → fd
          │
          ├─ [Pass 1] fill_pattern_bytes(buf, N, 0x00)
          │           lseek(fd, 0, SEEK_SET)
          │           write(fd, buf, N)
          │           fsync(fd)
          │
          ├─ [Pass 2] fill_pattern_bytes(buf, N, 0xFF)
          │           lseek(fd, 0, SEEK_SET)
          │           write(fd, buf, N)
          │           fsync(fd)
          │
          ├─ [Pass 3] fill_random_bytes(buf, N)
          │           lseek(fd, 0, SEEK_SET)
          │           write(fd, buf, N)
          │           fsync(fd)
          │
          ├─ ftruncate(fd, 0)  → zero the file length
          ├─ close(fd)
          └─ unlink(path)      → remove directory entry
                    │
                    ▼
              return 0 (success)
```

---

## Data Flow: `wipe-free` Command

```
openerase wipe-free /Volumes/USB
          │
          ▼
parse_and_execute(argc=3, argv=["openerase","wipe-free","/Volumes/USB"])
          │
          ▼  strcmp(argv[1], "wipe-free") == 0
cli.c → wipe_free_space("/Volumes/USB")
          │
          ├─ [Pass 1..3]:
          │    mkstemp("/Volumes/USB/.openerase_tmp_XXXXXX") → tmp_fd
          │    loop:
          │      fill_random_bytes(chunk, CHUNK_SIZE)
          │      write(tmp_fd, chunk, CHUNK_SIZE)
          │    until write() returns ENOSPC
          │    fsync(tmp_fd)
          │    close(tmp_fd)
          │    unlink(tmp_path)
          │
          └─ return 0 (success)
```

---

## Key Design Decisions

**Why separate shred.c and wipe.c?**
They have different threat models and different I/O patterns. Shred operates on known-size files; wipe-free operates until disk full. Keeping them separate makes each easier to audit and test.

**Why utils.c?**
The random byte generation and cache-bypass logic are security-critical and must be consistent. Centralizing them means there's one place to audit, one place to fix if a vulnerability is found.

**Why no global state?**
All functions receive their inputs as parameters. This makes the code testable (no global teardown between tests) and thread-safe by default.

**Why C11?**
Portable, mature, well-understood memory model. The `_Static_assert` and `<stdint.h>` types from C11 help catch issues at compile time. No runtime overhead from C++ abstractions.
