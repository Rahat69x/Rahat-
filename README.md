# CSE103: Structured Programming (C)

[![Language](https://img.shields.io/badge/Language-C99%20%2F%20C11-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Course](https://img.shields.io/badge/Course-CSE103%20Structured%20Programming-2ea44f)](https://github.com/rahat4528/CSE103-Structured-Programming)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)](https://github.com/rahat4528/CSE103-Structured-Programming)
[![Status](https://img.shields.io/badge/Status-Completed%20%26%20Maintained-brightgreen)](https://github.com/rahat4528/CSE103-Structured-Programming)

Comprehensive repository for the **CSE103: Structured Programming** course. This repository contains complete lab problem sets, algorithmic problem solutions, foundational C programming exercises, and multi-module software systems developed using structured programming principles.

---

## Course Curriculum & Topic Mapping

The coursework and projects demonstrate progression from foundational C programming constructs to complex modular system architecture:

```
CSE103: Structured Programming
├── 01. Basic I/O, Data Types & Operators
├── 02. Control Flow: Conditionals & Decisions
├── 03. Iteration & Loop Structures
├── 04. Functions & Modular Decomposition
├── 05. Array Processing, Traversal & Sorting
├── 06. Strings, Structs & File Streams
└── 07. Capstone Modular System Architectures
```

---

## Major Software Systems

Comprehensive, production-grade terminal applications demonstrating separation of concerns, modular design patterns, robust input validation, and defensive programming.

| System | Directory | Architecture & Key Highlights |
|---|---|---|
| **Console-Based ATM Simulation** | [`./c-atm-simulation`](./c-atm-simulation) | **ISO C11 modular core banking engine** implementing Features A1 to A22. Highlights include SHA-256 salted PIN authentication, simulated SMS OTP verification, multi-currency support (BDT/USD/EUR), transaction fraud anomaly detection, loan amortization schedules, bill payment processing, ATM cassette cash inventory, dispute resolution, and encrypted session logs. |
| **Blood Group Management System** | [`./c-blood-group-management`](./c-blood-group-management) | **Strict C99 modular medical dispatch & inventory platform** built on a linked list, a BST ID index, a phone hash table, 8 classified blood buckets, a request queue, a priority max heap, a bounded undo stack, a city graph (Dijkstra nearest-donor ranking), and a doubly linked history list with bidirectional pagination. |
| **Screen Sense** | [`./project_01_Screen_Sense`](./project_01_Screen_Sense) | **Digital wellbeing and ergonomics calculator**. Evaluates multi-device screen exposure (desktop, laptop, mobile), computes daily eye-strain risk factors, and provides behavioral usage guidelines. |
| **Code Quizzer** | [`./project_02_code_quizzer`](./project_02_code_quizzer) | **Interactive terminal knowledge assessment engine**. Generates randomized technical questionnaires evaluating core programming concepts, score calculation, and performance metrics. |

---

## Lab Problem Sets & Algorithm Catalog

A structured collection of algorithms, numerical methods, and foundational C programs addressing core computer science concepts:

### 1. Fundamentals, I/O & Mathematical Equations
* [`simple_calculator.c`](./simple_calculator.c) — Interactive multi-operation arithmetic calculator with division-by-zero protection.
* [`using_C_program_to_find_the_value_of_following_expression.c`](./using_C_program_to_find_the_value_of_following_expression.c) — Operator precedence and algebraic polynomial evaluation.
* [`Physics_formula_solver.c`](./Physics_formula_solver.c) — Kinematics and classical Newtonian motion equation solver (`v = u + at`, `s = ut + 0.5*a*t^2`).
* [`find_initial_velocity_using_motion_formula.c`](./find_initial_velocity_using_motion_formula.c) — Inverse kinematics formula solver for initial velocity.
* [`lea_ year_and_age_calculator.c`](./lea_%20year_and_age_calculator.c) — Gregorian calendar leap year validation logic and chronological date-of-birth age calculator.

### 2. Control Flow, Conditionals & Loops
* [`divisibility and even-odd checker.c`](./divisibility%20and%20even-odd%20checker.c) — Multi-rule parity, sign, and divisibility evaluation using modulo arithmetic.
* [`check_if_a_number_is_prime.c`](./check_if_a_number_is_prime.c) — Primality testing algorithm with `O(sqrt(n))` trial division.
* [`compute_the_factorial_of_a_number.c`](./compute_the_factorial_of_a_number.c) — Factorial accumulation (`n!`) handling boundary conditions.
* [`compute _the _sum_of_the_digits_of_an_integer.c`](./compute%20_the%20_sum_of_the_digits_of_an_integer.c) — Base-10 iterative digit extraction and summation.
* [`reverse_an_integer.c`](./reverse_an_integer.c) — Integer digit inversion algorithm using modulo-division recurrence.
* [`print_the_first_N_terms_of_the_fibonacci_series.c`](./print_the_first_N_terms_of_the_fibonacci_series.c) — Fibonacci recurrence generation.
* [`print_the_multiplication_table_of_a_number.c`](./print_the_multiplication_table_of_a_number.c) — Iterative formatted mathematical multiplication table generator.
* [`print_numbers_from_N_to_1_in_reverse_order.c`](./print_numbers_from_N_to_1_in_reverse_order.c) — Descending iterative loop counter.

### 3. Arrays, Search & Traversal
* [`Detect_whether_the_array_is_sorted.c`](./Detect_whether_the_array_is_sorted.c) — Single-pass linear scan verifying monotonic ascending/descending order (`O(n)`).
* [`Insert_5_numbers_into_the_array_from_the_keyboard.c`](./Insert_5_numbers_into_the_array_from_the_keyboard.c) — Fixed-size array allocation, keyboard stream ingestion, and linear traversal.

### 4. Discrete Logic & Theory
* [`discrete mathematics`](./discrete%20mathematics) — Graph representations, truth tables, and combinatorial logic solutions.

---

## Getting Started & Compilation Guide

### Prerequisites
* **Compiler**: GCC (MinGW-w64 on Windows, `build-essential` on Ubuntu/Debian, Xcode Command Line Tools on macOS) or Clang.
* **C Standards**: ISO C99 (`-std=c99`) or ISO C11 (`-std=c11`).
* **Build Utility**: GNU Make (`make` or `mingw32-make`).

---

### Building the ATM Simulation System

```bash
# Navigate to the ATM simulation module
cd c-atm-simulation

# Build using GNU Make
make

# Or compile manually with GCC
gcc -Wall -Wextra -std=c11 -O2 main.c atm.c -o atm_sim

# Run the simulation
./atm_sim          # Linux / macOS
.\atm_sim.exe      # Windows
```

---

### Building the Blood Group Management System

```bash
# Navigate to the blood group management directory
cd c-blood-group-management

# Build release target
make

# Run automated test suite
make test

# Launch the application
./build/bgms       # Linux / macOS
.\build\bgms.exe   # Windows
```

---

### Running Individual Lab Programs

Any standalone `.c` file can be compiled and executed directly:

```bash
# Example: Compile the prime checker
gcc -Wall -Wextra -std=c99 check_if_a_number_is_prime.c -o prime_check

# Execute
./prime_check      # Linux / macOS
.\prime_check.exe  # Windows
```

---

## Coding Standards & Architectural Principles

1. **Modular Decomposition**: Related functionality is partitioned into distinct `.h` interface headers and `.c` implementation files.
2. **Defensive Programming**: Functions validate all parameters, pointers, and memory allocations prior to execution.
3. **Memory Safety**: Dynamic allocations are strictly managed and paired with cleanup handlers.
4. **Clean Code & Portability**: Pure ISO C standards compliance allowing cross-platform execution on Windows, Linux, and macOS without non-standard dependencies.

---

## Author

* **Mahamudul Hasan Rahat**
  * GitHub: [@rahat4528](https://github.com/rahat4528)
  * Course: **CSE103 — Structured Programming Language**
