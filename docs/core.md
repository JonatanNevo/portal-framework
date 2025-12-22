# Portal Core

The core module provides common functionalities for all other modules.

## Features

### Utilities
 - [Logging](../core/portal/core/log.h)
 - [File System](../core/portal/core/files/file_system.h)
 - [Assertion](../core/portal/core/debug/assert.h)
 - [Timer](../core/portal/core/timer.h)
 - [Non Owning Buffer](../core/portal/core/buffer.h)
 - [Random](../core/portal/core/random/random.h)

### Core Systems

For detailed architecture documentation on the following systems, see [Core Systems Architecture](architecture/core-systems.md).

#### Jobs System
 - [Job System](../core/portal/core/jobs/job.h) - Work-stealing task scheduler with C++20 coroutines
 - Features: Job<T> coroutines, nested jobs, priority queues, work-stealing across worker threads
 - See architecture docs for execution flow, coroutine integration, and performance characteristics

#### Memory Allocators
Inspired by LLVM's support library allocators. Specialized allocators for performance-critical patterns:
 - [StackAllocator](../core/portal/core/memory/stack_allocator.h) - Linear allocator with marker-based bulk deallocation
 - [BufferedAllocator](../core/portal/core/memory/stack_allocator.h) - Multi-frame persistence for CPU-GPU pipelines
 - [PoolAllocator](../core/portal/core/memory/pool_allocator.h) - Fixed-size object pools with embedded freelist
 - [BucketPoolAllocator](../core/portal/core/memory/pool_allocator.h) - Variable-size allocations up to bucket size
 - See architecture docs for usage patterns and selection guide

#### Concurrency Primitives
Low-level synchronization primitives for multi-threaded environments:
 - [SpinLock](../core/portal/core/concurrency/spin_lock.h) - Non-reentrant spinlock with exponential backoff
 - [ReentrantSpinLock](../core/portal/core/concurrency/reentrant_spin_lock.h) - Same-thread multi-lock with hash-based ownership
 - [AssertionLock](../core/portal/core/concurrency/asserion_lock.h) - Debug-only zero-cost validation
 - See architecture docs for memory ordering details and usage guidelines

#### String ID System
Compile-time string hashing for efficient identifier lookups:
 - [StringId](../core/portal/core/strings/string_id.h) - 64-bit hash identity with debug string_view
 - [StringRegistry](../core/portal/core/strings/string_registry.h) - Permanent storage with deduplication
 - Features: Compile-time hashing on GCC/Clang, rapidhash integration, DenseMap compatibility
 - See architecture docs for design rationale and serialization patterns