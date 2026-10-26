# Blood Group Management System (C99 Standard)

A robust, menu-driven terminal management system in pure C99 for blood donors and blood requests, featuring transactional data structures: a singly linked list, a Binary Search Tree (BST), a hash table with separate chaining, an 8-group classified bucket index, a FIFO request queue, a priority max-heap, a bounded LIFO undo stack, a weighted city graph with Dijkstra's shortest-path algorithm, and a bounded doubly linked fulfillment history list with bidirectional pagination.

> **Educational Disclaimer:** This software is an educational project developed for academic computer science and data structures study. All donor names, contact numbers, and hospital records are entirely fictional. This system is **not** designed, certified, or intended for real-world medical, hospital, or blood-bank clinical operations.

---

## 1. Language & Compiler Standards

This project conforms strictly to the **ISO C99 standard** (`gcc -std=c99 -Wall -Wextra -pedantic -O2`) with **zero compiler warnings**.

Only the five standard C99 library headers are permitted and used throughout the entire codebase:
- `<stdio.h>`
- `<stdlib.h>`
- `<string.h>`
- `<ctype.h>`
- `<time.h>`

No external libraries, GUI frameworks, or non-standard headers (`stdbool.h`, `stdint.h`, `assert.h`, `errno.h`, `limits.h`) are utilized.

---

## 2. Architecture & Data Structures

Every data structure in this system performs a specific, non-redundant architectural responsibility:

| Data Structure | Module | Responsibility | Big-O Time |
|:---|:---|:---|:---|
| **Master Singly Linked List** | `donor.h`, `donor.c` | Owns all `Donor` heap records in registration sequence | Traversal: $O(n)$, Insert: $O(1)$ |
| **Binary Search Tree (BST)** | `bst.h`, `bst.c` | Secondary index by Unique Donor ID; sorted in-order traversal | Search/Insert: $O(\log n)$ avg, Delete: $O(\log n)$ |
| **Hash Table (Separate Chaining)** | `hashtable.h`, `hashtable.c` | Index by Phone Number; prime capacity 101, djb2 hash function | Search/Insert/Delete: $O(1)$ avg |
| **8-Group Classified Buckets** | `buckets.h`, `buckets.c` | Direct array of 8 linked bucket chains indexed by `BloodGroup` enum | Group Access: $O(1)$, Traversal: $O(k)$ |
| **FIFO Request Queue** | `queue.h`, `queue.c` | FIFO queue for normal-urgency blood requests | Enqueue/Dequeue: $O(1)$ |
| **Emergency Priority Max-Heap** | `heap.h`, `heap.c` | Priority queue for emergency blood requests (`CRITICAL` > `URGENT`) | Insert/Extract-Max: $O(\log n)$, Peek: $O(1)$ |
| **Bounded LIFO Undo Stack** | `undo.h`, `undo.c` | Bounded stack (capacity 20) for atomic rollback of request fulfillment | Push/Pop: $O(1)$, Oldest Eviction: $O(1)$ |
| **Weighted Undirected City Graph** | `citygraph.h`, `citygraph.c` | Feature F1: Bangladesh highway network and Dijkstra shortest path ranking | Dijkstra: $O(V^2 + E)$, Edge Insert: $O(1)$ |
| **Bounded Doubly Linked List** | `history.h`, `history.c` | Feature F2: Fulfilment audit history (capacity 200) with bidirectional pagination | Append/Pop-Tail: $O(1)$, Evict-Head: $O(1)$ |
| **Atomic File Persistence** | `storage.h`, `storage.c` | Human-readable serialization with atomic rename (`.tmp` $\to$ `.txt`) | Save/Load: $O(n)$ |

---

## 3. Project Directory Structure

```
c-blood-group-management/
├── common.h                # Global status codes and domain constants
├── common.c                # Status code human-readable descriptions
├── date.h                  # Strict ISO-8601 calendar date representation
├── date.c                  # Gregorian leap year rules and integer civil math
├── bloodgroup.h            # Blood group enum and compatibility definitions
├── bloodgroup.c            # 8x8 compatibility matrix and 90-day cooldown logic
├── utils.h                 # Safe I/O, integer parser, and domain field validators
├── utils.c                 # Input reading, overflow protection, and string trim
├── donor.h                 # Donor entity model and field validation rules
├── donor.c                 # Master singly linked list implementation
├── bst.h                   # Binary Search Tree index prototypes
├── bst.c                   # BST insert, lookup, deletion (3 cases), in-order traversal
├── hashtable.h             # Hash table with separate chaining prototypes
├── hashtable.c             # djb2 hash algorithm, bucket chaining, phone lookup
├── buckets.h               # 8-group classified bucket index prototypes
├── buckets.c               # Buckets insert, removal, and traversal
├── request.h               # Blood request domain model and urgency levels
├── request.c               # Request validation and field initialization
├── queue.h                 # FIFO request queue prototypes
├── queue.c                 # Request linked queue enqueue, dequeue, peek
├── heap.h                  # Max-heap priority queue prototypes
├── heap.c                  # Binary heap sift-up, sift-down, priority ordering
├── undo.h                  # Bounded LIFO undo stack prototypes
├── undo.c                  # Stack push, pop, oldest eviction at bottom
├── citygraph.h             # Feature F1: City graph and Dijkstra declarations
├── citygraph.c             # Highway graph, edge adjacency lists, Dijkstra algorithm
├── history.h               # Feature F2: Doubly linked fulfilment history prototypes
├── history.c               # Bounded doubly linked list, bidirectional pagination
├── storage.h               # Human-readable atomic persistence prototypes
├── storage.c               # Atomic file save, syntax parsing, index reconstruction
├── context.h               # Master SystemContext definition
├── context.c               # Context initialization and complete teardown
├── donor_service.h         # Transactional donor CRUD service interface
├── donor_service.c         # Cross-index synchronization for all donor mutations
├── request_service.h       # Request submission, prioritization, and matching service
├── request_service.c       # Dijkstra distance ranking, matching, undo, history logging
├── main.c                  # Interactive terminal menu (all 17 options)
├── Makefile                # Cross-platform build automation
├── .gitignore              # Git ignore rules for build artifacts and temporary files
├── README.md               # Complete system documentation
└── tests/
    ├── testing.h           # Zero-dependency assertion framework
    ├── test_phase0.c       # Phase 0: Foundations unit tests (139 assertions)
    ├── test_donor.c        # Phase 1: Donor linked list & validators (114 assertions)
    ├── test_bst.c          # Phase 2: BST ID index (62 assertions)
    ├── test_hashtable.c    # Phase 3: Phone hash table (36 assertions)
    ├── test_buckets.c      # Phase 4: Classified buckets (40 assertions)
    ├── test_queue.c        # Phase 5: FIFO request queue (46 assertions)
    ├── test_heap.c         # Phase 6: Emergency max-heap (25 assertions)
    ├── test_request_service.c # Phase 7: Request service & matching (19 assertions)
    ├── test_undo.c         # Phase 8: Bounded undo stack (44 assertions)
    ├── test_storage.c      # Phase 9: Atomic file persistence (20 assertions)
    └── test_features.c     # Phase 10: Features F1 & F2 (36 assertions)
```

---

## 4. Building & Running

All compilation artifacts and object binaries are generated inside the `build/` directory.

### Prerequisites
- GCC or Clang compiler supporting standard C99.
- PowerShell or bash terminal.

### Direct Compilation with GCC

```powershell
# Create build directory
if (!(Test-Path "build")) { New-Item -ItemType Directory -Path "build" }

# Compile the complete interactive application
gcc -std=c99 -Wall -Wextra -pedantic -O2 -I. common.c utils.c date.c bloodgroup.c donor.c bst.c hashtable.c buckets.c request.c queue.c heap.c undo.c citygraph.c history.c storage.c context.c donor_service.c request_service.c main.c -o build/bgms.exe

# Run the application
.\build\bgms.exe
```

### Running All 11 Automated Test Suites (581 Passing Assertions)

Run each unit test suite compiled with strict compiler flags:

```powershell
$srcs = @('common.c', 'utils.c', 'date.c', 'bloodgroup.c', 'donor.c', 'bst.c', 'hashtable.c', 'buckets.c', 'request.c', 'queue.c', 'heap.c', 'undo.c', 'citygraph.c', 'history.c', 'storage.c', 'context.c', 'donor_service.c', 'request_service.c')
$tests = @('test_phase0', 'test_donor', 'test_bst', 'test_hashtable', 'test_buckets', 'test_queue', 'test_heap', 'test_request_service', 'test_undo', 'test_storage', 'test_features')

foreach ($t in $tests) {
    & gcc -std=c99 -Wall -Wextra -pedantic -O2 -I. -Itests $srcs "tests/$t.c" -o "build/$t.exe"
    & "./build/$t.exe"
}
```

---

## 5. Interactive Terminal Menu Structure

```
=================================================================
         RED-PULSE: BLOOD GROUP MANAGEMENT SYSTEM               
        (Pure C99 Terminal Edition - All 10 Phases Active)       
=================================================================
System Date: YYYY-MM-DD | Donors: N | Pending Requests: P | Fulfilled: F
=================================================================
DONOR MANAGEMENT (Linked List, BST, Hash Table, Buckets):
  1. Register New Donor
  2. Search Donor by ID (BST Index)
  3. Search Donor by Phone (Hash Table Index)
  4. Update Donor Information
  5. Delete Donor Record
  6. Display All Donors (Master List Order)
  7. Display All Donors (Sorted by ID - BST In-Order)
  8. Display Donors by Blood Group (Classified Buckets)

BLOOD REQUESTS & PROCESSING (Priority Heap, FIFO Queue, Dijkstra Ranking):
  9. Submit Blood Request (Normal Queue or Emergency Max-Heap)
 10. Process Next Blood Request (Emergency First + Nearest-Donor Ranking)
 11. Undo Last Blood Request Fulfillment (Bounded LIFO Undo Stack)
 12. View Pending Blood Requests (Priority Heap & FIFO Queue)

FEATURE PACK (F1 City Graph & F2 Fulfilment History):
 13. Browse Fulfilment History (Doubly Linked List with Pagination)
 14. Query City Road Distance & Shortest Path (Dijkstra)

PERSISTENCE & UTILITIES:
 15. Save System State to Disk (Atomic File Persistence)
 16. Load System State from Disk (Reconstruct All Indexes)
 17. Query Blood Group Compatibility Matrix
  0. Exit System
=================================================================
```

---

## 6. Key Design Highlights

1. **Transactional Prepare/Commit:**
   - Pre-validation and pre-allocation occur prior to mutating any system data structures.
   - If an allocation fails or eligible donors are insufficient, state remains 100% untouched (`ST_ERR_NO_DONORS`).
2. **Dijkstra Nearest-Donor Ranking:**
   - In emergencies, candidate donors are ranked primarily by biological compatibility (exact match first), secondarily by shortest road distance computed via Dijkstra's algorithm from the patient's hospital city, and tertiarily by cooldown recovery.
3. **Atomic File Persistence:**
   - Serializes complete system state to a temporary file (`bgms_data.tmp`), flushes and closes it, then atomically renames it to `bgms_data.txt` to eliminate data corruption on unexpected power loss.
4. **Bounded Reversal & Auditing:**
   - Undo Stack stores up to 20 past fulfillments, reversing donor donation dates and re-enqueuing requests to their exact originating queues.
   - Fulfilment History stores up to 200 records in a doubly linked list supporting forward and backward page browsing.
