# Core Systems Architecture

This document provides architecture documentation for Portal Framework's foundational systems: the job system, memory allocators, concurrency primitives, and string ID system.

## Table of Contents

- [Job System](#job-system)
  - [Architecture Overview](#job-system-architecture-overview)
  - [Execution Flow](#job-execution-flow)
  - [Work-Stealing Mechanism](#work-stealing-mechanism)
  - [Coroutine Integration](#coroutine-integration)
  - [Performance Characteristics](#job-system-performance)
  - [Current Limitations](#job-system-limitations)
- [Memory Allocators](#memory-allocators)
  - [StackAllocator](#stackallocator)
  - [BufferedAllocator](#bufferedallocator)
  - [PoolAllocator](#poolallocator)
  - [BucketPoolAllocator](#bucketpoolallocator)
  - [When to Use Each](#allocator-selection-guide)
- [Concurrency Primitives](#concurrency-primitives)
  - [SpinLock](#spinlock)
  - [ReentrantSpinLock](#reentrantspinlock)
  - [Usage Guidelines](#concurrency-usage-guidelines)
- [String ID System](#string-id-system)
  - [Architecture](#string-id-architecture)
  - [Compile-Time Hashing](#compile-time-hashing)
  - [StringRegistry](#stringregistry)
  - [Hash Map Integration](#hash-map-integration)

---

## Job System

### Job System Architecture Overview

The Portal Framework job system is a work-stealing task scheduler built on C++20 coroutines.
It provides fork-join parallelism with nested job support, priority queues, and work distribution across worker threads. 
The design is inspired by modern game engine architectures and prioritizes CPU utilization, cache locality, and low-overhead task scheduling.

**Core Components:**

1. **Job&lt;T&gt; Coroutines** - Represent asynchronous units of work with typed results
2. **Scheduler** - Manages worker threads and distributes jobs
3. **WorkerQueue** - Per-worker priority queues with work-stealing support
4. **Counter** - Synchronization primitive for fork-join patterns

**Design Philosophy:**

Jobs should never block waiting.
Instead, they suspend and allow workers to process other work, maximizing CPU utilization.

**Key Files:**

- `core/portal/core/jobs/job.h`, `job.cpp` - Core job types and promises
- `core/portal/core/jobs/scheduler.h`, `scheduler.cpp` - Worker thread management

### Job Execution Flow

#### 1. Job Creation

When you create a `Job<T>` using a coroutine function (any function returning `Job<T>` with `co_await`/`co_return`), the C++20 coroutine machinery allocates a `JobPromise` object that stores the job's state:

- **Continuation handle** - What to resume when this job completes
- **Result storage pointer** - For non-void return values
- **Counter pointer** - For fork-join synchronization
- **Scheduler pointer** - To re-dispatch on suspension
- **Completion flag** - Tracks job status

**Critical Detail:** The job starts suspended and does NOT run until explicitly dispatched to the Scheduler.

#### 2. Job Dispatch

```cpp
// Example dispatch
Job<int> compute_task() {
    // Heavy computation
    co_return 42;
}

Scheduler scheduler(std::thread::hardware_concurrency() - 1);
int result = scheduler.wait_for_job(compute_task(), JobPriority::Normal);
```

When you call `scheduler.wait_for_job(my_job)`, the following sequence occurs:

**Dispatch Steps:**

1. Job is marked as dispatched (prevents double-dispatch and double-free)
2. Job's promise receives its scheduler and counter pointers
3. Job handle is enqueued in the calling thread's WorkerQueue
4. A Counter is created with `count=1` (or incremented if multiple jobs)
5. Calling thread enters the `wait_for_counter()` loop, actively executing jobs while waiting 

#### 3. Worker Thread Loop

Each worker thread runs `worker_thread_loop()` continuously. On every iteration:

```
┌─────────────────────────────────────────┐
│  1. Check per-worker job cache          │
│     └─> If non-empty: pop and execute   │
│                                         │
│  2. If cache empty: bulk-dequeue from   │
│     local queue (High → Normal → Low)   │
│                                         │
│  3. If local empty: attempt steal from  │
│     random victim's stealable queue     │
│                                         │
│  4. If steal fails: try global queue    │
│     (used by main thread)               │
│                                         │
│  5. If all empty: yield thread          │
└─────────────────────────────────────────┘
```

**Job Cache Optimization:**

The cache batches queue operations (which involve atomic operations) and turns them into simple array accesses.
Workers fill their cache in bulk using `try_pop_bulk(max_count=cache_size)` and execute jobs one by one from the cache.

This reduces atomic operations in typical workloads.

### Work-Stealing Mechanism

#### Two-Queue Architecture

Every worker has TWO queues:

1. **Local Queue** - Producer-side optimized (owning worker pushes/pops with minimal contention)
2. **Stealable Queue** - Consumer-side optimized (other workers can steal from it)

This dual-queue design reduces contention.
Workers mostly work on their local queue, only occasionally moving work to the stealable side, 
and thieves only access the stealable queue.

#### Migration Strategy

Every `STEAL_CHECK_INTERVAL` iterations, a worker calls `migrate_jobs_to_stealable()`. 
If the local queue depth exceeds `THRESHOLD`, it moves `MOVE_COUNT` from local to stealable.

#### Steal Algorithm

When a worker has no local work:

1. Pick a random victim worker
2. Call `attempt_steal()` on victim's WorkerQueue
3. Bulk-dequeue from victim's stealable queue (High → Normal → Low priority order)
4. Steal up to `max_count` jobs
5. If steal succeeds, execute stolen jobs; if fails, try another victim

### Coroutine Integration

#### JobPromise and the Coroutine Protocol

The `JobPromise` implements the C++20 coroutine promise concept. 
When `execute_job()` runs, it performs `co_await job.promise()`, triggering the `JobPromise::JobAwaiter` protocol:

```cpp
struct JobAwaiter {
    bool await_ready() {
        return promise.completed;
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) {
        // Set who to resume after this job completes
        promise.continuation = continuation;
        // Return job's handle to resume it
        return promise.handle;
    }

    void await_resume() {}
};
```

The coroutine machinery resumes the job by calling `job.handle.resume()`. The job's coroutine body executes until it hits:

1. **`co_return value`** - Triggers `return_value()`, sets `completed=true`, stores result, calls `final_suspend()`
2. **`co_await SuspendJob()`** - Suspends the job and re-dispatches it to the scheduler
3. **`co_await child_job`** - Suspends until child completes

#### Nested Jobs and Automatic Continuations

When a parent job does `co_await child_job`, 
the `JobAwaiter::await_suspend()` sets `child_job`'s continuation to point to the parent's coroutine handle.
When the child completes (in `FinalizeJob::await_suspend()`), it returns the parent's handle, 
causing the parent to resume **immediately on the same worker thread**.

**Zero-Overhead Nested Jobs:**

- No thread wakeups
- No queue operations
- Just coroutine handle swaps
- Parent resumes exactly where it left off
- CPU cache stays hot (parent and child data likely still in L1/L2)

```cpp
Job<int> parent() {
    int a = co_await compute_a();  // Spawns child, suspends parent
    int b = co_await compute_b();  // Child completes, resumes parent inline
    co_return a + b;               // No context switches involved
}
```

#### SuspendJob: Voluntary Yielding

When a job executes `co_await SuspendJob()`, the `SuspendJob::await_suspend()` implementation:

- Re-dispatches the job to the scheduler's queue (job is incomplete, just yielding)
- If job has a counter, clears the blocking flag and notifies waiting threads
- Returns the continuation handle (back to worker's event loop)

**Use Case:** Long-running jobs can voluntarily yield execution to allow other work to proceed, 
preventing monopolization of a worker thread.

```cpp
Job<void> long_task() {
    for (int i = 0; i < 1000000; ++i) {
        // Do some work
        process_item(i);

        // Yield every 1000 iterations to be fair to other jobs
        if (i % 1000 == 0) {
            co_await SuspendJob();
        }
    }
}
```

#### FinalizeJob and Completion

When a job reaches its end (`co_return`), the `JobPromise::final_suspend()` returns a `FinalizeJob` awaiter. 
`FinalizeJob::await_suspend()` performs cleanup:

- Decrements the associated Counter using `fetch_sub` with release semantics
- If counter reaches zero, clears the blocking flag and notifies waiting threads
- Returns the continuation handle (parent job or `noop_coroutine` if no parent)

### Job System Performance

#### Concurrency Control with Counters

The `Counter` struct uses:

- `std::atomic<size_t> count` - Number of in-flight jobs
- `std::atomic_flag blocking` - Wait/notify to avoid busy-waiting

**Hybrid Wait Strategy:**

The calling thread doesn't just sleep waiting for jobs - it actively participates in executing work until there's truly nothing left to do.

`wait_for_counter()` uses the following approach:

1. While `counter > 0`, keep executing jobs via `worker_thread_iteration()` (keep working while waiting)
2. If iteration returns `EmptyQueue`, test_and_set the blocking flag
3. If still `> 0`, `blocking.wait(true)`
4. When notified or counter reaches zero, clear flag and continue
 

#### Performance Characteristics

**Job Statistics (when `ENABLE_JOB_STATS` is defined):**

The JobStats system tracks:
- Work executed/submitted counts and timings (min/max/avg)
- Steal attempts and success rates
- Queue depths (sampled every 1000 iterations)
- Idle time percentages
- Load imbalance (coefficient of variation)
- Cache hit rates (local vs stealable vs global queue)

### Job System Limitations

#### Current Limitations

1. **No Priority Preemption** - A running low-priority job won't be preempted by a high-priority job arrival. Priority only affects dequeue order.

2. **No Work Stealing from Local Queues** - Only stealable queues are stolen from. Work must be migrated before it's stealable (reduces contention but adds latency).

3. **No Job Cancellation** - Once dispatched, a job runs to completion (though it can be yielded and deprioritized).

4. **Single-Level Priority** - No dynamic priority adjustment or priority inheritance (e.g., a low-priority job holding a resource needed by a high-priority job won't automatically inherit higher priority).

5. **Fixed Cache Size** - The job cache size is set at scheduler construction and never changes.

#### Future Enhancement Opportunities

**Memory Management:**

Currently, job promises are dynamically allocated using malloc and free, in the future I want to move to a faster approach to memory handling, probably a stack allocator per in flight frame

**Work Stealing from Local Queues:**

Allow stealing directly from local queues as a fallback when stealable queues are empty. This would increase contention but improve load balancing in scenarios where work isn't being migrated fast enough.

**Job Cancellation:**

Add cancellation tokens that jobs can check periodically. Challenges include:
- Ensuring partial work is properly cleaned up
- Handling nested jobs (do children get cancelled automatically?)
- Memory management for cancelled job promises

**Priority Inheritance:**

Track resource dependencies and automatically boost job priority when a lower-priority job is blocking higher-priority work.

---

## Memory Allocators

The Portal Framework provides specialized allocators that complement the global mimalloc allocator.
Each targets specific allocation patterns where general-purpose allocators are suboptimal. 
The design philosophy is: **use the right allocator for the right pattern**.

### StackAllocator

**Type:** Linear/Bump Allocator for Temporary Data

**Implementation:** (`core/portal/core/memory/stack_allocator.h`)

StackAllocator maintains a contiguous `std::vector<uint8_t>` buffer and a 'top' marker (byte offset). Allocation is trivial: check if `top + size <= buffer.size()`, return `buffer.data() + top`, increment top by size. This is O(1) with no synchronization.

#### Key Feature: Marker-Based Bulk Deallocation

```cpp
class StackAllocator {
public:
    using marker = size_t;

    marker get_marker() const { return top; }
    void free_to_marker(marker m) { top = m; }
};
```

`get_marker()` returns the current top value. After allocating many objects, `free_to_marker(marker)` simply resets top to that marker value, instantly freeing everything allocated since.

**Performance:** This is orders of magnitude faster than calling `free()` on each allocation individually.

#### Usage Pattern: Per-Frame Allocations

```cpp
StackAllocator frame_alloc(1_MB);

void process_frame() {
    auto marker = frame_alloc.get_marker();

    // Allocate temporary per-frame data
    auto* entities = frame_alloc.alloc<EntityData>(entity_count);
    auto* transforms = frame_alloc.alloc<Matrix4x4>(entity_count);
    auto* culling_results = frame_alloc.alloc<bool>(entity_count);

    // ... use data throughout frame ...

    render_scene(entities, transforms, culling_results, entity_count);

    // Free everything at once - single integer assignment
    frame_alloc.free_to_marker(marker);
}
```

#### Individual free() Support

Individual `free()` is supported but inefficient. It stores allocations in an `std::unordered_map<void*, size_t>` mapping pointers to sizes. On `free()`, it looks up the size, erases the map entry, and decrements top by size.

**Critical Limitation:** This only works correctly in LIFO order (stack semantics). Freeing in arbitrary order causes fragmentation and incorrect top calculations.

#### Important Limitations

**No Alignment Guarantees:**

StackAllocator does not track alignment. Allocations are tightly packed with no padding. If you allocate a `char` followed by a `double`, the double may be misaligned.

**Solution for Aligned Data:**

For types requiring specific alignment (SSE/AVX vectors, GPU buffers, cache-line-aligned data), use `aligned_alloc()` or `posix_memalign()` instead, or implement custom alignment logic.

**clear() Doesn't Update Allocations Map:**

`free_to_marker()` is by design for performance. The marker is a snapshot of the buffer state. The allocations map becomes irrelevant after marker-based deallocation.

**Tradeoff:** After `free_to_marker()`, DO NOT call `free()` on individual allocations made after that marker, as the map is now inconsistent. Use marker-based deallocation exclusively, or `clear()` which resets both top and the map.

### BufferedAllocator

**Type:** Multi-Frame Persistence (Double/Triple Buffering)

**Implementation:** (`core/portal/core/memory/stack_allocator.h` - template)

`BufferedAllocator<N>` wraps N independent StackAllocators and rotates through them using `swap_buffers()`. The current buffer is accessed via `stack_index` (which mod N on swap).

#### Use Case: CPU-GPU Pipeline

The critical use case is multi-frame data persistence for CPU-GPU pipelines:

```
Frame N:   Write GPU staging data to buffer 0
           GPU reads frame N-1 data from buffer 1

Frame N+1: swap_buffers() -> stack_index = (stack_index + 1) % N
           Clear newly-active buffer
           Write GPU staging data to buffer 1 (in double buffering) or buffer 2 (in triple)
           GPU reads frame N data from buffer 0
```

#### Example: Vertex Upload

```cpp
DoubleBufferedAllocator staging_alloc(2_MB);  // BufferedAllocator<2>

void render_frame() {
    // Swap to next buffer and clear it
    staging_alloc.swap_buffers();

    // Allocate vertex data in current buffer
    auto& current = staging_alloc.get_current_allocator();
    Vertex* vertices = current.alloc<Vertex>(vertex_count);

    // Fill vertex data
    fill_vertices(vertices, vertex_count);

    // Upload to GPU (GPU will read asynchronously)
    gpu_upload(vertices, vertex_count * sizeof(Vertex));

    // Buffer persists until swap_buffers() cycles back (2 or 3 frames)
}
```

### PoolAllocator

**Type:** Fixed-Size Object Pool with Embedded Freelist

**Implementation:** (`core/portal/core/memory/pool_allocator.h`)

`PoolAllocator<T, C, L>` pre-allocates a fixed array of `C * sizeof(T)` bytes and manages it using an embedded freelist.

#### Embedded Freelist Design

The freelist is embedded: each free block stores a pointer to the next free block **inside the free block itself**. This is why `sizeof(T) >= sizeof(void*)` is required.

**Initialization (in `clear()`):**

```
Pool array divided into sizeof(T)-byte blocks:
┌─────┬─────┬─────┬─────┬─────┐
│ B0  │ B1  │ B2  │ B3  │ B4  │
└─────┴─────┴─────┴─────┴─────┘
  │      │      │      │      │
  ├─────>├─────>├─────>├─────>nullptr

head points to B0
Each block stores pointer to next block
```

**Allocation:**

1. Pop head
2. Dereference head to get next free block
3. Move head to next
4. Placement-new construct T at the popped memory
5. Return pointer

**Deallocation:**

1. Call destructor on T
2. Cast memory to `void**`
3. Store current head in the first `sizeof(void*)` bytes of freed memory
4. Update head to point to this freed block

**Both operations are O(1)** - no malloc, just pointer manipulation.

#### Thread Safety

The `L` template parameter defaults to `SpinLock`. Every `alloc()` and `free()` operation locks the `lock_object`.

**Optimization Opportunities:**

- **Single-threaded:** Substitute a no-op lock type to eliminate locking overhead, such as `AssertionLock`
  ```cpp
  PoolAllocator<Entity, 1024, AssertionLock> single_threaded_pool;
  ```

- **High contention, longer critical sections:** Substitute `std::mutex`
  ```cpp
  PoolAllocator<NetworkPacket, 256, std::mutex> network_pool;
  ```

#### Critical Safety Issue: clear()

**WARNING:** `clear()` does NOT call destructors on allocated objects.

`clear()` rebuilds the freelist by iterating over the pool array and resetting the next-pointers, then sets head to the first block and `full=false`. It does NOT call destructors.

**Consequences:**

If you have 10 active allocations and call `clear()`, those 10 objects' destructors are NOT called. This causes:
- Resource leaks (heap memory, file handles, mutexes held by those objects)
- Undefined behavior (objects think they're alive but their memory is now in the freelist)

**Safe Usage:**

Only call `clear()` when:
1. The pool is completely empty (all objects freed via `free()`), OR
2. All allocated objects are trivially destructible (no resources to clean up)

For proper cleanup, iterate over all active objects and call `free()` on each BEFORE calling `clear()`.

#### When to Use PoolAllocator

**Ideal Use Cases:**

- Fixed-size objects (entities, particles, network packets)
- High allocation/deallocation frequency
- Known maximum count (bounded resource)
- Objects with short, similar lifetimes

**Example: Entity Component System:**

```cpp
PoolAllocator<Entity, 10000> entity_pool;

Entity* spawn_entity() {
    return entity_pool.alloc(/* constructor args */);
}

void destroy_entity(Entity* e) {
    entity_pool.free(e);  // Destructor called, memory returned to pool
}
```

### BucketPoolAllocator

**Type:** Variable-Size Allocations up to Bucket Size

**Implementation:** (`core/portal/core/memory/pool_allocator.h`)

`BucketPoolAllocator<B, C, L, check_allocations>` allocates fixed-size B-byte buckets from a pool of C buckets. Unlike PoolAllocator which allocates typed objects, this allocates raw memory (`void*`).

#### Design

**Bucket Size:** The bucket size `B` is the maximum allocation size. All allocations return a B-byte bucket **regardless of requested size** (there's no size parameter in `alloc()` - you always get B bytes).

**Use Cases:**

- Small message queues
- String buffers
- Protocol packets
- Any variable-sized data with a known maximum size

**Caller Responsibility:** Track how much of the bucket is actually used.

#### Allocation Tracking

The `check_allocations` template parameter (defaults `false`) enables allocation tracking via `std::atomic<size_t> allocated_buckets`.

- When `true`: `alloc()` increments counter, `free()` decrements it
- `get_allocation_size()` returns current count
- Adds overhead (atomic operations) but useful for debugging leaks

**Example: Message Queue:**

```cpp
BucketPoolAllocator<512, 1024, SpinLock, true> message_pool;  // 512KB total

void* allocate_message(size_t msg_size) {
    assert(msg_size <= 512);  // Must fit in bucket
    return message_pool.alloc();  // Always returns 512-byte bucket
}

void free_message(void* msg) {
    message_pool.free(msg);
}

// Check for memory leaks
size_t leaked_messages = message_pool.get_allocation_size();
if (leaked_messages > 0) {
    LOG_ERROR("Leaked {} messages", leaked_messages);
}
```

### Allocator Selection Guide

| Use Case                                 | Recommended Allocator | Reason                                              |
|------------------------------------------|-----------------------|-----------------------------------------------------|
| Per-frame temporary data                 | StackAllocator        | Bulk deallocation, cache-friendly, zero overhead    |
| GPU staging buffers                      | BufferedAllocator     | Multi-frame persistence for async GPU reads         |
| Fixed-size objects (entities, particles) | PoolAllocator         | O(1) alloc/free, cache-friendly LIFO reuse          |
| Variable-size messages (up to max)       | BucketPoolAllocator   | Pooling benefits with size flexibility              |
| Long-lived, variable-size data           | mimalloc (global)     | General-purpose allocator handles these well        |
| Large allocations (>1MB)                 | mimalloc (global)     | Specialized allocators don't help with large blocks |

**Performance Hierarchy (for their respective use cases):**

1. **StackAllocator (with marker-based deallocation)** - Fastest, but most constrained
2. **PoolAllocator** - Very fast, requires fixed size
3. **BucketPoolAllocator** - Fast, wastes space for small allocations
4. **mimalloc** - Excellent general-purpose performance
5. **Individual free() on StackAllocator** - Slowest specialized allocator option

---

## Concurrency Primitives

The Portal Framework provides two spinlock implementations for protecting shared data in multi-threaded environments.

### SpinLock

**Type:** Non-Reentrant, Exponential Backoff Spinlock

**Implementation:** (`core/portal/core/concurrency/spin_lock.h`)

SpinLock uses `std::atomic_flag locked` (initialized to clear). 
This is the most primitive atomic type - guaranteed lock-free on all platforms.

#### Operations

**try_lock():**

```cpp
bool try_lock() {
    return !locked.test_and_set(std::memory_order_acquire);
}
```

Call `test_and_set(acquire)` on the flag. Returns `false` if flag was already set (lock held), `true` if flag was clear (lock acquired).

**Memory Ordering:** The acquire ordering ensures all subsequent reads see memory written before the previous unlock.

**lock() with Exponential Backoff:**

```cpp
void lock() {
    if (try_lock()) return;

    size_t backoff = 1;
    while (!try_lock()) {
        for (size_t i = 0; i < backoff; ++i) {
            std::this_thread::yield();
        }
        backoff = std::min(backoff * 2, 1024);
    }
}
```

1. First `try_lock()` - if succeeds, done
2. Enter exponential backoff loop:
   - yield() for `backoff` iterations
   - Try lock again
   - If succeeds, return
   - Double backoff up to max 1024

**unlock():**

```cpp
void unlock() noexcept {
    locked.clear(std::memory_order_release);
}
```

The release ordering ensures all prior writes are visible to the next acquirer.

#### SpinLock vs std::mutex

**std::mutex:**

- Involves kernel syscalls (futex on Linux, WaitOnAddress on Windows)
- Expensive: ~1-10 microseconds per lock/unlock
- Waiting threads sleep, yielding CPU to other work

**SpinLock:**

- Userspace-only, no syscalls
- Fast: ~10-50 nanoseconds for uncontended locks
- Waiting threads spin, consuming CPU cycles

**Performance Comparison:**

For very short critical sections (< 100ns), spinlocks are **orders of magnitude faster**.

#### When to Use SpinLock

**Ideal Scenarios:**

- Critical section is a few instructions (< 100ns)
- Lock is rarely contended (most `try_lock()` succeed immediately)
- You have spare CPU cores (spinning doesn't steal cycles from other work)

**Examples:**

```cpp
SpinLock freelist_lock;
void* pop_freelist() {
    std::lock_guard lock(freelist_lock);
    // 5-10 instructions to pop from freelist
    return head;
}
```

#### When NOT to Use SpinLock

**Bad Scenarios:**

- Critical section is long (milliseconds) or does I/O
- High contention (many threads competing for the same lock)
- Limited CPU resources (embedded systems, oversubscribed machines)

**Anti-Pattern:**

```cpp
SpinLock file_lock;
void write_log(const std::string& msg) {
    std::lock_guard lock(file_lock);
    file.write(msg);  // I/O operation - NEVER use spinlock
}
```

Use `std::mutex` instead - I/O operations are orders of magnitude longer than spinlock overhead.

### ReentrantSpinLock

**Type:** Reentrant Spinlock (Same Thread Can Lock Multiple Times)

**Implementation:** (`core/portal/core/concurrency/reentrant_spin_lock.h`)

`ReentrantSpinLock<T>` (T defaults to `uint32_t`) uses:

- `std::atomic<size_t> locked_thread` - Stores hash of owning thread's ID (0 = unlocked)
- `T ref_count` - **Non-atomic** counter tracking lock depth

#### Operations

**try_lock():**

```cpp
bool try_lock() {
    size_t thread_id = hash_thread_id();
    bool acquired = false;

    // Reentrancy check
    if (locked_thread.load(std::memory_order_relaxed) == thread_id) {
        acquired = true;
    } else {
        // Try acquire with relaxed CAS
        size_t expected = 0;
        acquired = locked_thread.compare_exchange_strong(expected, thread_id,
                                                         std::memory_order_relaxed);
    }

    if (acquired) {
        ++ref_count;
        std::atomic_thread_fence(std::memory_order_acquire);  // Fence after increment
        return true;
    }
    return false;
}
```

1. Hash current thread ID
2. If `locked_thread` already equals this hash, mark as acquired (reentrancy)
3. Otherwise, try `compare_exchange_strong()` with relaxed ordering (compare to 0, set to thread_id if equal)
4. If acquired (either path), increment `ref_count` then issue acquire fence for proper synchronization

**lock():**

Similar to `try_lock()` but loops with `yield()` until acquisition succeeds. Reentrancy check happens first (relaxed ordering - we're the owning thread, no inter-thread visibility needed).

**unlock():**

```cpp
void unlock() {
    std::atomic_thread_fence(std::memory_order_release);
    --ref_count;
    if (ref_count == 0) {
        locked_thread.store(0, std::memory_order_relaxed);
    }
}
```

1. **Release fence** (ensure all prior writes are visible before unlock)
2. Decrement `ref_count`
3. If `ref_count` reaches 0, release lock (set `locked_thread=0` with relaxed - fence already provides ordering)

#### Why Non-Atomic ref_count is Safe

**Key Insight:** Only the thread that owns the lock modifies `ref_count`. Other threads check `locked_thread` (atomic) and see it's owned, so they never touch `ref_count`.

This avoids atomic overhead on the counter. The fence operations ensure memory ordering is correct when ownership transfers between threads.

**Ordering:**

- `locked_thread` provides the cross-thread synchronization
- `ref_count` is single-thread access only (protected by ownership)
- Fences ensure proper happens-before relationships

### AssertionLock

**Type:** Debug-Only Lock for Development

**Implementation:** (`core/portal/core/concurrency/asserion_lock.h`)

AssertionLock is a zero-cost lock for production builds that catches threading bugs during development. It uses a simple `volatile bool` and asserts if contention is detected.

#### How It Works

```cpp
class AssertionLock {
public:
    void lock() {
        PORTAL_ASSERT(!locked, "Cannot lock an already locked AssertionLock");
        locked = true;
    }

    void unlock() {
        PORTAL_ASSERT(locked, "Cannot unlock an already unlocked AssertionLock");
        locked = false;
    }

private:
    volatile bool locked;
};
```

**Key Properties:**

- **Not a real lock** - provides no synchronization or mutual exclusion
- **Debug-only** - catches incorrect assumptions about single-threaded access
- **Zero production cost** - macros compile to nothing when `PORTAL_ENABLE_ASSERTS` is disabled
- **Fast failure** - immediately asserts if contention occurs, making bugs obvious

#### When to Use AssertionLock

**Ideal for:**
- Code you believe is single-threaded but want to verify
- Development/testing to catch unexpected concurrent access
- Documenting lock points that are stripped in release builds

**Example:**

```cpp
class RenderQueue {
    AssertionLock queue_lock;
    std::vector<DrawCall> calls;

public:
    void submit(DrawCall call) {
        ASSERT_LOCK_GUARD(queue_lock);  // Compiles to nothing in release
        calls.push_back(call);
    }
};
```

If your assumption is wrong and multiple threads call `submit()`, you'll get an immediate assertion failure in debug builds. In release builds, the lock disappears entirely (zero overhead).

**When NOT to Use:**

- Any code that actually needs thread safety
- Production builds where you need synchronization
- Code where contention is expected

### Concurrency Usage Guidelines

#### Choosing Between Spinlock and Mutex

| Scenario                                     | Recommendation | Reason                                       |
|----------------------------------------------|----------------|----------------------------------------------|
| Freelist push/pop (5-10 instructions)        | SpinLock       | Critical section < 50ns                      |
| Incrementing counters                        | SpinLock       | Single atomic operation                      |
| Queue push/pop (lock-free queue unavailable) | SpinLock       | Short critical section, high frequency       |
| File I/O                                     | std::mutex     | Long critical section, I/O latency dominates |
| Database queries                             | std::mutex     | Very long critical section                   |
| Protecting complex data structures           | std::mutex     | Critical section > 1us                       |
| Anything with memory allocation              | std::mutex     | malloc/free can take microseconds            |

#### Choosing Between SpinLock and ReentrantSpinLock

| Scenario                                | Recommendation    | Reason                                       |
|-----------------------------------------|-------------------|----------------------------------------------|
| Simple data protection, flat call graph | SpinLock          | No reentrancy needed, lower overhead         |
| Recursive functions                     | ReentrantSpinLock | May reenter same lock                        |
| Callbacks with unknown call graph       | ReentrantSpinLock | Caller may already hold lock                 |
| Performance-critical hot path           | SpinLock          | Avoid hash overhead if reentrancy not needed |

---

## String ID System

The Portal Framework string ID system provides compile-time/fast string hashing for efficient identifier lookups while maintaining human-readable debug information.

### String ID Architecture

#### Core Concept

`StringId` is a struct with two fields:

```cpp
struct StringId {
    uint64_t id;                // The 64-bit hash (from rapidhash)
    std::string_view string;    // Human-readable string for debugging
};
```

**Critical Design Decision:** Two StringId instances are equal if their `id` fields match, **regardless of what `string` says**. The string is purely for diagnostics.

This allows:

1. **Deserialization from disk:** Receive a hash over the network, construct `StringId(hash)`, look up the string in the registry for logging
2. **Compile-time constants:** `STRING_ID("player")` computes the hash at compile time and embeds it as a literal in the binary
3. **Efficient comparisons:** Just compare 64-bit integers, no string operations

#### Equality and Hashing

```cpp
bool operator==(const StringId& other) const {
    return id == other.id;  // String is NOT compared
}

std::size_t hash<StringId>::operator()(const StringId& id) const {
    return id.id;  // Hash is already well-distributed
}
```
### Hashing

The string ID system uses rapidhash, a high-performance 64-bit hash function based on wyhash.

**Properties:**
- **Fast:** Optimized for short strings (typical identifier length: 5-30 characters)
- **High quality:** Excellent avalanche properties (small input changes cause large output changes)
- **Deterministic:** Same string always hashes to the same value across platforms/compilers
- **Platform-independent:** Uses only standard C++ operations

**Source:** [rapidhash on GitHub](https://github.com/Nicoshev/rapidhash)

#### STRING_ID() Macro

**Implementation:** (`core/portal/core/strings/string_id.h`)

```cpp
#define STRING_ID(string) \
    portal::StringId(hash::rapidhash(string), std::string_view(string))
```

**On GCC/Clang:** `hash::rapidhash()` is `constexpr`, so `STRING_ID("foo")` is evaluated at compile time. The generated assembly contains the 64-bit hash as an immediate value - **no runtime computation**.

```cpp
constexpr auto player_id = STRING_ID("player");
// Compiled to: mov rax, 0x1234567890abcdef  (actual hash value)
```

**On MSVC:** `constexpr rapidhash` doesn't work (MSVC lacks constexpr support for 128-bit multiplication used internally). The hash is computed at runtime but highly optimized/inlined.

### StringRegistry

**Type:** Singleton for Permanent String Storage

**Implementation:** (`core/portal/core/strings/string_registry.h`)

The StringRegistry functions as a singleton storing all registered strings.
By owning the string allocations, it enables use of `string_view` throughout the codebase
and simplifies serialization by centralizing all string IDs during startup and shutdown.

#### String Deduplication

If you create 1000 StringId instances all with `STRING_ID("player")`:

- **Compile-time IDs:** All point to the same string literal in the binary's data section (no duplication)
- **Runtime IDs:** First call stores in registry, subsequent calls return the same registry entry (deduplicated)

### StringId Constructors

#### 1. Macros (recommended)

```cpp
auto id = STRING_ID("foo");
auto id = STRING_ID(str);
```

- Hash computed at compile time (GCC/Clang) or inlined at runtime (MSVC)
- String points to binary's string literal
- For runtime strings computes the hash and stores in the registry

#### 2. Hash-Only (Deserialization)

```cpp
StringId id(hash);
```

- Only have the hash (e.g., loaded from binary file or network packet)
- Look up string in registry via `find()`
- If not found, logs error and sets string = "Invalid"
- **Used for network communication and save files**

**Example: Network Deserialization:**

```cpp
void deserialize_entity(BinaryStream& stream) {
    uint64_t type_hash = stream.read_uint64();
    StringId type_id(type_hash);  // Lookup in registry

    if (type_id.string == INVALID_STRING_VIEW) {
        LOG_ERROR("Unknown entity type hash: 0x{:x}", type_hash);
        return;
    }

    LOG_INFO("Spawning entity: {}", type_id);  // Logs "id(\"player\")"
}
```

---

## Future Enhancements

**Job System:**
- Priority preemption for time-critical tasks
- Job cancellation with proper cleanup semantics

**Memory Allocators:**
- Alignment-aware StackAllocator
- Thread-safe registry for PoolAllocators to track allocations globally
- Automatic pool size tuning based on usage patterns

**Concurrency:**
- Lock-free data structures where applicable (queues, stacks)
- Hazard pointers for safe memory reclamation in lock-free code
- Reader-writer locks for scenarios with read-heavy workloads

**String IDs:**
- Binary serialization/deserialization of the entire string registry
- Compile-time hash on all platforms (including MSVC)
- Frozen map with all compile-time known strings for zero-cost lookup
- Thread-safe StringRegistry implementation