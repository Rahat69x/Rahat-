# Blood Group Management System (C99 Standard)

A robust, menu-driven terminal management system in pure C99 for blood donors and blood requests, featuring transactional data structures: a singly linked list, dual Binary Search Trees, a hash table, a request queue, a priority max heap, an undo stack, a city graph, and a doubly linked history list.

> **Educational Disclaimer:** This software is an educational project developed for academic computer science study. All sample donor names and contact numbers are entirely fictional. This system is **not** designed, certified, or intended for real-world medical, hospital, or blood-bank clinical operations.

---

## 1. Language & Compiler Standards

This project conforms strictly to the **ISO C99 standard** (`-std=c99 -Wall -Wextra -pedantic`) with zero compiler warnings. While the repository root prerequisites reference C11 for other projects, all C99 code compiles cleanly and seamlessly under modern C11-capable compilers (such as GCC and Clang).

Only the five standard C99 library headers are permitted and used throughout the entire codebase:
- `<stdio.h>`
- `<stdlib.h>`
- `<string.h>`
- `<ctype.h>`
- `<time.h>`

No external libraries, GUI frameworks, or non-standard headers (`stdbool.h`, `stdint.h`, `assert.h`, `errno.h`, `limits.h`) are utilized.

---

## 2. Phase 0 Implementation Status: Foundations

The following core modules are fully implemented and verified with zero compiler warnings:

| Module | Files | Responsibility |
|:---|:---|:---|
| **Common** | `common.h`, `common.c` | Global `Status` enumeration, domain capacity constants, human-readable status descriptions. |
| **Date & Calendar** | `date.h`, `date.c` | Strict ISO-8601 parsing (`YYYY-MM-DD`), Gregorian leap year rules, integer days-from-civil and civil-from-days conversion, sentinel `0000-00-00` (`never`). |
| **Blood Group** | `bloodgroup.h`, `bloodgroup.c` | 8 ABO/Rh group enumeration, string conversion, biological compatibility matrix, 90-day cooldown eligibility evaluation. |
| **Utilities & I/O** | `utils.h`, `utils.c` | Overflow-protected integer parsing (rejects >9 digits), whitespace trimming, ASCII text validation (disallows `\|`), phone number validation, safe string copying. |
| **System Context** | `context.h`, `context.c` | Master `SystemContext` lifecycle management, monotonic ID and sequence counters. |
| **Testing Harness** | `tests/testing.h`, `tests/test_phase0.c` | Zero-dependency assertion framework (`CHECK`, `CHECK_EQ_INT`, `CHECK_STR_EQ`) verifying all foundational units. |

---

## 3. Project Directory Structure

```
c-blood-group-management/
├── common.h            # Global status codes and domain constants
├── common.c            # Status code descriptions
├── date.h              # Calendar date representation and integer civil math
├── date.c              # Civil date arithmetic and leap year algorithms
├── bloodgroup.h        # Blood group enum and compatibility definitions
├── bloodgroup.c        # 8x8 compatibility matrix and eligibility logic
├── utils.h             # Safe I/O, integer parser, and domain validators
├── utils.c             # Safe input reading, overflow guards, and string trim
├── context.h           # Shared SystemContext structure and lifecycle prototypes
├── context.c           # Context initialization and clean teardown
├── main.c              # Application entry point and interactive demonstration
├── Makefile            # Portable build configuration (Linux, macOS, Windows)
├── .gitignore          # Build artifacts and runtime data ignore rules
├── README.md           # Project documentation and specifications
└── tests/
    ├── testing.h       # Standalone assertion framework (no assert.h)
    └── test_phase0.c   # Automated unit test suite (139 assertions)
```

---

## 4. Building & Running

All compilation artifacts and object files are generated inside the `build/` directory.

### Prerequisites
- GCC or Clang compiler supporting C99.
- `make` or `mingw32-make` (or run direct `gcc` commands below).

### Build Targets

```bash
# Build the application
make

# Run the automated unit test suite
make test

# Memory diagnostic target (reports availability cleanly)
make valgrind

# Address and Undefined Behavior sanitizer target
make sanitize

# Clean all build artifacts
make clean
```

On Windows with MinGW, `mingw32-make` can be used identically:
```powershell
mingw32-make
mingw32-make test
mingw32-make clean
```

### Direct GCC Compilation (without make)

```bash
# Create build directory if needed:
# Linux/macOS: mkdir -p build
# Windows cmd: if not exist build mkdir build

# Compile the main executable
gcc -std=c99 -Wall -Wextra -pedantic -O2 -I. -Itests common.c utils.c date.c bloodgroup.c context.c main.c -o build/bgms

# Compile and run unit tests
gcc -std=c99 -Wall -Wextra -pedantic -O2 -I. -Itests common.c utils.c date.c bloodgroup.c context.c tests/test_phase0.c -o build/test_phase0
./build/test_phase0
```

### Execution
```bash
./build/bgms            # Linux / macOS
.\build\bgms.exe        # Windows
```

---

## 5. Storage & Persistence Defaults

The system state persists to a pipe-delimited text file named `bgms_data.txt` located in the current working directory, validated with a 32-bit FNV-1a checksum and staging context rollback protection.
