# Contributing to OpenErase

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/OpenErase.git`
3. Create a feature branch: `git checkout -b feature/your-feature-name`
4. Make changes, run tests, commit
5. Open a Pull Request against `main`

---

## Code Style

**Language:** C11 strictly. No C99-isms, no compiler extensions.

**Formatting:**
- 4-space indentation (no tabs)
- `snake_case` for all identifiers (functions, variables, types)
- `UPPER_CASE` for macros and enum values
- Opening braces on the same line: `if (x) {`
- One blank line between functions
- Max line length: 100 characters

**Examples:**

```c
// Good
int secure_shred(const char *path, WipeMode mode) {
    if (path == NULL) {
        return -1;
    }
    // ...
    return 0;
}

// Bad — wrong brace style, mixed case, no null check
int SecureShred(const char *path, WipeMode mode)
{
    // ...
}
```

**Headers:**
- Every `.c` file includes its own `.h` file first
- Use include guards (`#ifndef FOO_H / #define FOO_H / #endif`)
- No circular includes — `utils.h` must not include `shred.h`

**Memory:**
- Every `malloc` must have a matching `free` on all code paths (success and error)
- Use `secure_zero()` before `free()` on any buffer that held sensitive data
- No VLAs — enforced by `-Werror=vla`

**Error handling:**
- Check the return value of every syscall (`open`, `write`, `fsync`, `unlink`, etc.)
- Return `-1` and preserve `errno` on failure
- Print errors to `stderr`, not `stdout`

---

## PR Workflow

1. **One PR per feature or fix** — keep diffs focused and reviewable
2. **Tests required** — every new function needs at least one test
3. **Build must pass** — `cmake -B build && cmake --build build` with zero warnings
4. **Tests must pass** — `cd build && ctest --output-on-failure`
5. **Describe the why** — PR description should explain the motivation, not just the what

**Commit message format:**
```
type: short description (under 50 chars)

Optional longer explanation of why this change was made.
Wrap at 72 characters.
```

Types: `feat`, `fix`, `refactor`, `test`, `docs`, `chore`

---

## Security Review Requirement

**Any change to `shred.c`, `wipe.c`, or `utils.c` requires a security review before merge.**

These files implement the core data destruction logic. A bug here (missing `fsync`, wrong pass count, weak PRNG) defeats the entire purpose of the tool and could give users a false sense of security.

For security-sensitive changes, your PR description must include:
- What the change does
- What threat it addresses or how it affects the security guarantee
- How you verified correctness (manual testing, code review, references)

If you're unsure whether your change is security-sensitive, err on the side of flagging it.

---

## Adding a New Wipe Algorithm

To add a new overwrite algorithm (e.g., a custom pass sequence):

1. **Add the enum value** to `WipeMode` in `include/shred.h`:
   ```c
   typedef enum {
       WIPE_SINGLE  = 1,
       WIPE_DOD     = 3,
       WIPE_GUTMANN = 35,
       WIPE_MYALGO  = N,   // your new value
   } WipeMode;
   ```

2. **Implement the pass sequence** in `src/shred.c` — add a case in the pass loop for `WIPE_MYALGO`

3. **Document the algorithm** in `docs/ALGORITHMS.md`:
   - Source / standard the algorithm comes from
   - Pass sequence with byte patterns
   - When to use it vs other algorithms

4. **Add CLI flag support** in `src/cli.c` — expose it via `--mode myalgo` argument

5. **Write tests** in `test/test_shred.c`

6. **Security review required** (see above) — new algorithms touch core engine code

---

## What Not to Contribute

- **`--skip-passes` or `--fast` flags** that reduce pass count below the selected algorithm — this breaks the security guarantee
- **`rand()` / `srand()`** anywhere in the codebase — use `/dev/urandom` or `arc4random_buf()`
- **VLAs** — banned, `-Werror=vla` will reject them at compile time
- **Platform-specific code outside `utils.c`** — engines must stay platform-agnostic
- **Dependencies** — OpenErase has zero runtime dependencies by design; keep it that way

---

## Reporting Security Issues

Do not open a public GitHub issue for security vulnerabilities.

Email the maintainer directly or use GitHub's private security advisory feature:
`Security` tab → `Report a vulnerability`

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)
