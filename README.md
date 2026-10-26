# Rahat's C Programming & System Projects

Welcome to the central repository for C programming projects, algorithmic solutions, and system simulations for the CSE 103 course and advanced software architecture.

---

## Project Index

### Major Projects

| Project | Location | Description |
|---|---|---|
| **Console-Based ATM Simulation** | [`./c-atm-simulation`](./c-atm-simulation) | Comprehensive modular ATM simulation in ISO C11 implementing Core Banking + Advanced Features A1 to A22 (Salted PINs, Simulated SMS OTP, Fraud Detection Engine, Multi-Currency BDT/USD/EUR, Loan Amortization, Recurring Bills, Cassette Inventory, Disputes, and Audit Logging). |
| **Blood Group Management System** | [`./c-blood-group-management`](./c-blood-group-management) | Menu-driven terminal system in C99 for blood donor and request management, built on a linked list, two BSTs, a hash table, a queue, a max heap and an undo stack. (in development) |
| **Screen Sense** | [`./project_01_Screen_Sense`](./project_01_Screen_Sense) | Digital wellbeing screen time tracker evaluating daily PC, laptop, and phone usage thresholds. |
| **Code Quizzer** | [`./project_02_code_quizzer`](./project_02_code_quizzer) | Interactive terminal-based knowledge quiz program assessing programming concepts. |

---

### Coursework & Algorithm Implementations

A collection of focused problem solutions exploring variables, conditional branches, iterative loops, functions, and array manipulation in C:

- **Mathematical & Numerical Programs:**
  - [`check_if_a_number_is_prime.c`](./check_if_a_number_is_prime.c): Primality test algorithm.
  - [`compute_the_factorial_of_a_number.c`](./compute_the_factorial_of_a_number.c): Factorial computation via iterative accumulation.
  - [`compute _the _sum_of_the_digits_of_an_integer.c`](./compute%20_the%20_sum_of_the_digits_of_an_integer.c): Digit summation using modulo arithmetic.
  - [`divisibility and even-odd checker.c`](./divisibility%20and%20even-odd%20checker.c): Modulo-based parity and divisibility evaluation.
  - [`print_the_first_N_terms_of_the_fibonacci_series.c`](./print_the_first_N_terms_of_the_fibonacci_series.c): Fibonacci sequence generation.
  - [`print_the_multiplication_table_of_a_number.c`](./print_the_multiplication_table_of_a_number.c): Iterative multiplication table generator.
  - [`print_numbers_from_N_to_1_in_reverse_order.c`](./print_numbers_from_N_to_1_in_reverse_order.c): Descending loop counter.
  - [`reverse_an_integer.c`](./reverse_an_integer.c): Integer digit reversal via base-10 arithmetic.
  - [`using_C_program_to_find_the_value_of_following_expression.c`](./using_C_program_to_find_the_value_of_following_expression.c): Algebraic expression evaluation.
  - [`discrete mathematics`](./discrete%20mathematics): Graph theory and discrete logic concepts.

- **Scientific & Utility Calculators:**
  - [`simple_calculator.c`](./simple_calculator.c): Terminal calculator with basic arithmetic operations.
  - [`Physics_formula_solver.c`](./Physics_formula_solver.c): Kinematics and classical motion physics solver.
  - [`find_initial_velocity_using_motion_formula.c`](./find_initial_velocity_using_motion_formula.c): Initial velocity equation solver.
  - [`lea_ year_and_age_calculator.c`](./lea_%20year_and_age_calculator.c): Gregorian leap year validator and chronological age calculator.

- **Arrays & Data Manipulation:**
  - [`Detect_whether_the_array_is_sorted.c`](./Detect_whether_the_array_is_sorted.c): Array order verification algorithm.
  - [`Insert_5_numbers_into_the_array_from_the_keyboard.c`](./Insert_5_numbers_into_the_array_from_the_keyboard.c): Keyboard array input ingestion and traversal.

---

## Getting Started

### Prerequisites
- GCC / Clang compiler supporting the ISO C11 standard (`-std=c11`).
- `make` or `mingw32-make` (optional, for automated project builds).

### Building & Running the ATM Simulation
```bash
# Navigate to the integrated ATM simulation directory
cd c-atm-simulation

# Build using make (or: gcc -Wall -Wextra -std=c11 main.c atm.c -o atm_sim)
make

# Run the terminal application
./atm_sim       # On Linux/macOS
.\atm_sim.exe   # On Windows
```

### Building & Running the Blood Group Management System
```bash
# Navigate to the blood group management system directory
cd c-blood-group-management

# Build using make or mingw32-make
make            # Or: mingw32-make

# Run the terminal application
./build/bgms         # On Linux/macOS
.\build\bgms.exe     # On Windows
```

### Running Standalone C Programs
```bash
gcc -Wall -Wextra simple_calculator.c -o simple_calculator
./simple_calculator
```

---

## Educational Disclaimer

All projects and simulations within this repository are developed strictly for educational and academic purposes. Simulated banking transactions, card numbers, passcodes, and currency conversions carry no monetary value.
