# OpenErase — Wipe Algorithms

## Overview

OpenErase implements multiple overwrite algorithms. More passes = more time, but not always more security. The right choice depends on the storage medium.

| Algorithm | Passes | Use Case |
|-----------|--------|----------|
| Single Random | 1 | SSDs, NVMe, modern HDDs |
| DoD 5220.22-M | 3 | General purpose, compliance |
| Gutmann | 35 | Legacy HDDs, regulatory requirements |

---

## Algorithm Details

### 1. Single Random Pass

**Passes:** 1
**Pattern:** Cryptographically random bytes

```
Pass 1: [random bytes from /dev/urandom]
```

**When to use:**
- Modern SSDs and NVMe drives — additional passes don't help due to wear leveling
- When speed matters and the threat model doesn't require multi-pass
- Flash storage of any kind

**Implementation:**
```c
fill_random_bytes(buf, file_size);
write_and_sync(fd, buf, file_size);
```

---

### 2. DoD 5220.22-M (3-pass)

**Passes:** 3
**Standard:** US Department of Defense, National Industrial Security Program

```
Pass 1: 0x00 0x00 0x00 0x00 ...  (all zeros)
Pass 2: 0xFF 0xFF 0xFF 0xFF ...  (all ones)
Pass 3: [random bytes]
```

**When to use:**
- General-purpose secure deletion
- Compliance with US government security standards
- Spinning HDDs where single-pass is insufficient
- The recommended default for OpenErase

**Why this pattern:**
The alternating 0/1/random sequence ensures that any magnetic remanence from pass 1 is overwritten by the opposite polarity in pass 2, with a final random pass to destroy any pattern-based recovery.

**Implementation:**
```c
// Pass 1
fill_pattern_bytes(buf, size, 0x00);
lseek(fd, 0, SEEK_SET);
write(fd, buf, size); fsync(fd);

// Pass 2
fill_pattern_bytes(buf, size, 0xFF);
lseek(fd, 0, SEEK_SET);
write(fd, buf, size); fsync(fd);

// Pass 3
fill_random_bytes(buf, size);
lseek(fd, 0, SEEK_SET);
write(fd, buf, size); fsync(fd);
```

---

### 3. Gutmann 35-pass

**Passes:** 35
**Source:** Peter Gutmann, "Secure Deletion of Data from Magnetic and Solid-State Memory" (USENIX Security 1996)

The 35 passes use specific byte patterns designed to defeat the magnetic force microscopy (MFM) and scanning tunneling microscopy (STM) recovery techniques of the mid-1990s.

**Pass sequence:**
```
Passes  1- 4: Random
Pass    5:    0x55 0x55 0x55
Pass    6:    0xAA 0xAA 0xAA
Pass    7:    0x92 0x49 0x24
Pass    8:    0x49 0x24 0x92
Pass    9:    0x24 0x92 0x49
Pass   10:    0x00 0x00 0x00
Pass   11:    0x11 0x11 0x11
Pass   12:    0x22 0x22 0x22
Pass   13:    0x33 0x33 0x33
Pass   14:    0x44 0x44 0x44
Pass   15:    0x55 0x55 0x55
Pass   16:    0x66 0x66 0x66
Pass   17:    0x77 0x77 0x77
Pass   18:    0x88 0x88 0x88
Pass   19:    0x99 0x99 0x99
Pass   20:    0xAA 0xAA 0xAA
Pass   21:    0xBB 0xBB 0xBB
Pass   22:    0xCC 0xCC 0xCC
Pass   23:    0xDD 0xDD 0xDD
Pass   24:    0xEE 0xEE 0xEE
Pass   25:    0xFF 0xFF 0xFF
Pass   26:    0x92 0x49 0x24
Pass   27:    0x49 0x24 0x92
Pass   28:    0x24 0x92 0x49
Pass   29:    0x6D 0xB6 0xDB
Pass   30:    0xB6 0xDB 0x6D
Pass   31:    0xDB 0x6D 0xB6
Passes 32-35: Random
```

**When to use:**
- Legacy spinning HDDs (pre-2001 era)
- Strict regulatory or compliance requirements that specifically mandate Gutmann
- When you have time and want maximum theoretical coverage

**When NOT to use:**
- SSDs, NVMe, flash storage — Gutmann provides no benefit and causes unnecessary wear
- Modern HDDs with perpendicular recording — most passes are irrelevant to current encoding

**Note from Gutmann (2001 addendum):**
> "If you're using a drive which uses [modern encoding techniques], then the Gutmann method isn't applicable to it. You'd be better off using a few random passes."

---

## PRNG: Why Not rand()

OpenErase uses `/dev/urandom` (POSIX) or `arc4random_buf()` (BSD/macOS), never `rand()` / `srand()`.

**Problem with rand():**
- Linear congruential generator — output is predictable given the seed
- If an attacker knows the seed (e.g., via timestamp), they can reconstruct every "random" byte written
- This defeats the purpose of a random overwrite pass

**Solution:**
```c
// Good — cryptographically unpredictable
arc4random_buf(buf, len);          // macOS / BSD
// or
int fd = open("/dev/urandom", O_RDONLY);
read(fd, buf, len);                // Linux / POSIX

// Bad — never use these
srand(time(NULL));
rand();
```

---

## SSD vs HDD: Why It Matters

### Spinning HDDs
- Data is written to a fixed physical location on the platter
- Overwriting that location replaces the magnetic state
- Multi-pass overwrite is meaningful — each pass destroys residual magnetism
- DoD 3-pass is appropriate

### SSDs / NVMe / Flash
- **Wear leveling:** The controller deliberately spreads writes across cells to prevent any single cell from wearing out. Writing to "the same location" may write to a different physical cell each time.
- **Over-provisioning:** SSDs reserve 7-28% extra space invisible to the OS. Data in this reserve area cannot be targeted by software.
- **Flash Translation Layer (FTL):** The controller maps logical addresses to physical NAND blocks. A software overwrite doesn't guarantee the same physical block is written.

**Practical implication:**
On an SSD, a single random overwrite pass is as effective as 35 passes from the OS perspective. The FTL may not even write your overwrite to the same cells. For SSDs, the effective mitigation is **full-disk encryption from the start** (so deleted data is already ciphertext) or **ATA Secure Erase** (a hardware-level command).

OpenErase still writes overwrite passes to SSDs because:
1. It's better than nothing
2. Many users don't know what storage type they have
3. It correctly handles SSDs in pass-through mode (no FTL)

---

## Recommended Defaults

| Storage Type | Recommended Mode | Reason |
|-------------|-----------------|--------|
| SSD / NVMe | `WIPE_SINGLE` | FTL makes extra passes ineffective; minimize wear |
| HDD (modern) | `WIPE_DOD` | 3 passes sufficient for perpendicular recording |
| HDD (legacy, pre-2001) | `WIPE_GUTMANN` | Gutmann patterns relevant for older encoding |
| USB flash drive | `WIPE_SINGLE` | Flash storage, same as SSD |
| RAM disk / tmpfs | `WIPE_SINGLE` | Volatile storage; one pass is enough |

**Default when unspecified:** `WIPE_DOD` (3-pass) — good balance of speed and coverage for the most common case (modern HDD or unknown storage type).
