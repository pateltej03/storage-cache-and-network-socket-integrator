# 🗃️ mdadm Networked RAID Emulator with Caching and Custom Memory Management

This project is a multi-phase C-based implementation of a JBOD (Just a Bunch Of Disks) RAID-like emulator system, completed as a semester-long assignment for CMPSC311 (Introduction to Systems Programming). It spans multiple progressive modules that emulate memory allocation, caching, local JBOD access, and full client-server networking.

---

## 📁 Project Structure

### 1. `mm.c` – Custom Memory Allocator

Implements a manual heap manager using:

-   **Explicit free lists** for free block management.
-   **Header/Footer Optimization** to reduce metadata overhead.
-   **Coalescing and Splitting** to optimize memory reuse.
-   **Bit Masking** to track block allocation and previous block allocation.

**Supported Functions:**

-   `malloc`, `free`, `calloc`, `realloc`
-   Heap checker with `mm_checkheap()`

> This custom memory allocator manages dynamic memory with a structure similar to standard `malloc/free`, but with improved memory efficiency using a footer optimization strategy.

---

### 2. `cache.c` – LRU Block Cache for Disk I/O

Implements an in-memory cache layer with:

-   **LRU Replacement Policy**
-   **Cache Lookup/Insert/Update Functions**
-   Performance metrics like hit rate

**Key Methods:**

-   `cache_create`, `cache_destroy`
-   `cache_lookup`, `cache_insert`, `cache_update`
-   `cache_enabled`, `cache_print_hit_rate`

> This cache significantly improves read/write latency by intercepting frequently accessed disk blocks.

---

### 3. `mdadm.c` – JBOD Disk Emulator Interface

Simulates block-level read/write access across multiple disks.

**Responsibilities:**

-   Mount/unmount the JBOD
-   Convert logical byte addresses to disk/block/byte tuples
-   Read and write across disk/block boundaries
-   Integrate cache for faster access

**Key Logic:**

-   Uses `encode_op()` to format 32-bit JBOD operation codes
-   Implements `mdadm_read()` and `mdadm_write()` to chunk operations
-   Coordinates with cache layer for reads/writes

> Effectively mimics how a RAID controller maps file system requests to physical disk sectors.

---

### 4. `net.c` – Network Communication Layer

Implements a TCP client to communicate with the JBOD server using a custom binary protocol.

**Protocol Format (JBOD Packet):**
| Bytes | Field | Description |
|-------|--------------|--------------------------------------|
| 0-1 | length | Total packet size |
| 2-5 | opcode | Operation code |
| 6-7 | return code | Result of operation |
| 8-263 | block | Optional 256-byte data block |

**Functions Implemented:**

-   `jbod_connect()` / `jbod_disconnect()`
-   `jbod_client_operation()` with `send_packet()` and `recv_packet()`
-   `nread()` / `nwrite()` for safe I/O

> Enables client-server JBOD interaction via a network, allowing for distributed disk operations over TCP.

---

## 🧪 Testing and Debugging

-   Debugging via `#define DEBUG` in `mm.c`
-   Heap consistency is verified via `mm_checkheap()`
-   Cache hit rates tracked and printed using `cache_print_hit_rate()`
-   Network behavior tested using `jbod_server` in verbose mode

---

## 🧠 Concepts Demonstrated

-   Pointer arithmetic and typecasting
-   Memory alignment and packing
-   Networking with sockets in C (IPv4, TCP)
-   Bitwise manipulation for encoding operation instructions
-   Explicit memory management and garbage collection via coalescing
-   Caching mechanisms with LRU eviction
-   Systems-level error handling and boundary validation

---

## 💡 Future Improvements

-   Convert LRU cache to use a doubly-linked list for faster eviction
-   Improve realloc efficiency by prioritizing in-place expansion
-   Add performance benchmarks comparing custom allocator with `malloc`

---

## 🧠 Let’s Connect!

**Tej Jaideep Patel**  
B.S. Computer Engineering  
📍 Penn State University  
✉️ tejpatelce@gmail.com  
📞 814-826-5544

---
