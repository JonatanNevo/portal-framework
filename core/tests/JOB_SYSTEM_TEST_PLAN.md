# Job System Test Plan

Comprehensive test plan for the Portal Framework job system covering all features and edge cases.

## Test Status Legend
- ✓ = Implemented
- ⧗ = Partially implemented
- ✗ = Not yet implemented

---

## 1. **Job Basics** ✓

### 1.1 Single Job Execution ✓
- ✓ **Test**: Single job completes successfully
- ✓ **Test**: Job<void> returns void properly
- ✓ **Test**: Job can be created and destroyed without execution

### 1.2 Job Return Values ✓
- ✓ **Test**: Job<T> returns value via result()
- ✓ **Test**: result() returns std::expected with value
- ✓ **Test**: result() returns error when job not dispatched
- ✓ **Test**: Move-only return types
- N/A **Test**: Copy-only return types (not supported - requires move semantics)

### 1.3 SuspendJob Awaitable ✓
- ✓ **Test**: Job suspends and resumes correctly
- ✓ **Test**: Multiple suspensions in single job
- ✓ **Test**: Suspension at different points in job execution

### 1.4 FinalizeJob ✓
- ✓ **Test**: final_suspend decrements Counter::count
- ⧗ **Test**: Job handle is destroyed after finalization (tested indirectly)
- ✓ **Test**: Counter unblocks when last job finalizes
- ⧗ **Test**: FinalizeJob notifies with notify_all (tested indirectly)

### 1.5 Job Lifecycle ✓
- ✓ **Test**: JobBase move constructor transfers handle
- ✓ **Test**: JobBase move assignment transfers handle
- ✓ **Test**: Destructor destroys handle only if not dispatched

---

## 2. **Counter (Job Completion Tracking)**

### 2.1 Basic Operations ✗
- ✗ **Test**: Counter::count increments on dispatch_jobs
- ✗ **Test**: Counter::count decrements on job completion
- ✗ **Test**: Counter reaches zero after all jobs complete

### 2.2 Blocking/Unblocking ✗
- ✗ **Test**: blocking flag prevents multiple threads from blocking
- ✗ **Test**: Counter unblocks when count reaches zero
- ✗ **Test**: notify_all wakes all waiting threads
- ✗ **Test**: SuspendJob clears blocking flag to wake waiting threads

### 2.3 Memory Ordering ✗
- ✗ **Test**: fetch_add uses release ordering
- ✗ **Test**: fetch_sub uses acq_rel ordering
- ✗ **Test**: load uses acquire ordering in wait loops
- ✗ **Test**: Blocking flag uses proper acquire/release

---

## 3. **Scheduler**

### 3.1 Creation & Configuration ⧗
- ✓ **Test**: Create scheduler with 0 worker threads (main thread only)
- ✓ **Test**: Create scheduler with 1 worker thread
- ✗ **Test**: Create scheduler with multiple worker threads
- ✗ **Test**: Create scheduler with -1 (hardware_concurrency - 1)
- ✗ **Test**: Scheduler with more threads than hardware cores

### 3.2 Job Distribution ✗
- ✗ **Test**: Jobs distributed across worker threads via WorkerQueue
- ✗ **Test**: Job executed on main thread when all workers full
- ✗ **Test**: try_distribute_to_worker skips current worker's queue
- ✗ **Test**: Single job on single-threaded scheduler
- ✗ **Test**: More jobs than worker queue capacity

### 3.3 wait_for_jobs API ⧗
- ✓ **Test**: wait_for_jobs(std::span<JobBase>) returns when complete
- ✗ **Test**: wait_for_jobs(std::span<Job<T>>) returns vector of results
- ✗ **Test**: wait_for_jobs(std::tuple<Job<T>...>) returns tuple of results
- ✗ **Test**: wait_for_jobs(Job<T>...) variadic overload
- ✗ **Test**: wait_for_job(Job<void>) returns void
- ✗ **Test**: wait_for_job(Job<T>) returns T
- ✗ **Test**: Handles empty span
- ✗ **Test**: Multiple consecutive wait_for_jobs calls
- ✗ **Test**: Thread that calls wait_for_jobs executes jobs from global queue

### 3.4 dispatch_jobs API ✗
- ✗ **Test**: dispatch_jobs with Counter tracks completion
- ✗ **Test**: dispatch_jobs without Counter (fire-and-forget)
- ✗ **Test**: dispatch_job single job overload
- ✗ **Test**: Dispatched jobs have dispatched=true flag set

### 3.5 Nested wait_for_jobs Support ⧗
- ⧗ **Test**: Worker thread can call wait_for_jobs (nested)
- ✗ **Test**: Worker processes own queue during nested wait
- ✗ **Test**: Thread-local tl_current_worker_queue set correctly
- ✗ **Test**: Multiple levels of nested wait_for_jobs
- ✗ **Test**: Worker doesn't distribute to own queue

### 3.6 Destructor & Cleanup ✗
- ✗ **Test**: Destructor sets has_work flags on all WorkerQueues
- ✗ **Test**: Worker threads stop gracefully via stop_token
- ✗ **Test**: Scheduler destruction with pending jobs
- ✗ **Test**: tl_current_worker_queue cleared on thread exit

---

## 4. **WorkerQueue & ConcurrentQueue**

### 4.1 Basic Operations ✗
- ✗ **Test**: try_enqueue succeeds when space available
- ✗ **Test**: try_enqueue fails when queue full (BLOCK_SIZE limit)
- ✗ **Test**: try_dequeue returns job when available
- ✗ **Test**: try_dequeue returns nullptr when empty

### 4.2 Wait/Notify ✗
- ✗ **Test**: has_work.wait() blocks until job available
- ✗ **Test**: Worker thread wakes on has_work.notify_one()
- ✗ **Test**: has_work flag set before notify
- ✗ **Test**: has_work flag cleared after processing
- ✗ **Test**: Destructor sets has_work to wake threads for shutdown

### 4.3 Per-Worker Queue Isolation ✗
- ✗ **Test**: Each worker only dequeues from own queue
- ✗ **Test**: Jobs distributed round-robin to available workers
- ✗ **Test**: Worker can steal from own queue in nested wait_for_jobs
- ✗ **Test**: BLOCK_SIZE=2 limits queue capacity correctly

---

## 5. **Nested Jobs (Jobs within Jobs)** ✓

### 5.1 Basic Nesting ✓
- ✓ **Test**: Outer job spawns inner jobs
- ✓ **Test**: Verify all inner jobs execute
- ✓ **Test**: Verify execution order (outer before inner)

### 5.2 Deep Nesting ✗
- ✗ **Test**: 3+ levels of nested jobs
- ✗ **Test**: Each level spawns multiple sub-jobs
- ✗ **Test**: Verify execution order at all levels

### 5.3 Nested Job Edge Cases ✗
- ✗ **Test**: Outer job with no inner jobs
- ✗ **Test**: Inner job spawns its own sub-jobs
- ✗ **Test**: Nested jobs with suspensions at each level

---

## 6. **Multi-Threading**

### 6.1 Thread Distribution ⧗
- ✓ **Test**: Jobs execute on multiple threads (verifies distinct threads)
- ✗ **Test**: Verify thread affinity settings
- ✗ **Test**: Load balancing across worker threads

### 6.2 Race Conditions ✗
- ✗ **Test**: Concurrent Counter::count modifications
- ✗ **Test**: Concurrent push/pop on pending_jobs queue
- ✗ **Test**: Concurrent WorkerQueue access
- ✗ **Test**: Multiple threads calling wait_for_jobs simultaneously

### 6.3 Thread Safety ✗
- ✗ **Test**: ConcurrentQueue thread safety
- ✗ **Test**: Atomic operations correctness (Counter, has_work)
- ✗ **Test**: Blocking flag memory ordering
- ✗ **Test**: Thread-local tl_current_worker_queue isolation

---

## 7. **Eager Workers**

### 7.1 Work Stealing (job.cpp:41-47) ✗
- ✗ **Test**: Suspended job immediately fetches next job
- ✗ **Test**: Worker thread executes job eagerly
- ✗ **Test**: Multiple threads stealing work
- ✗ **Test**: No work available for eager worker

---

## 8. **Edge Cases & Error Handling**

### 8.1 Empty/Null Cases ✗
- ✗ **Test**: Empty span passed to wait_for_jobs
- ✗ **Test**: pop_job returns nullptr when empty
- ✗ **Test**: Scheduler with no worker threads (0 workers)
- ✗ **Test**: result() called on Job<void> returns VoidType error

### 8.2 Large Scale ✗
- ✗ **Test**: 1000+ jobs in single span
- ✗ **Test**: Very deep call stacks
- ✗ **Test**: Large tuple of jobs (50+ jobs)

### 8.3 Stress Tests ✗
- ✗ **Test**: Rapid job creation/completion cycles
- ✗ **Test**: Long-running jobs mixed with short jobs
- ✗ **Test**: All threads suspended simultaneously
- ✗ **Test**: Worker queue saturation (exceed BLOCK_SIZE)

### 8.4 Memory & Lifetime ✗
- ✗ **Test**: No memory leaks with many jobs
- ✗ **Test**: Coroutine handle lifetime management
- ✗ **Test**: JobBase destroyed without dispatch doesn't leak
- ✗ **Test**: Job moved before dispatch transfers ownership correctly

---

## 9. **Task&lt;T&gt; System**

### 9.1 Return Values ✓
- ✓ **Test**: Task&lt;void&gt; execution
- ✓ **Test**: Task&lt;T&gt; returns value
- ✓ **Test**: Nested Task&lt;T&gt; with value propagation

### 9.2 Task Chaining ✗
- ✗ **Test**: Multiple co_await in sequence
- ✗ **Test**: Task continuation mechanism
- ✗ **Test**: FinalAwaiter returns correct continuation

### 9.3 Task Lifetime ✗
- ✗ **Test**: Task move constructor
- ✗ **Test**: Task move assignment
- ✗ **Test**: Task destruction before execution
- ✗ **Test**: Task double-move protection

### 9.4 Exception Handling ✗
- ✗ **Test**: unhandled_exception() logging
- ✗ **Test**: Exception in nested Task

---

## 10. **Integration & Real-World Scenarios**

### 10.1 Patterns ✗
- ✗ **Test**: Fan-out pattern (1 job spawns N jobs)
- ✗ **Test**: Pipeline pattern (chain of dependent jobs)
- ✗ **Test**: Parallel execution of independent jobs

### 10.2 Performance ✗
- ✗ **Test**: Overhead vs raw std::thread
- ✗ **Test**: Scheduler efficiency with different worker counts
- ✗ **Test**: Context switch overhead

### 10.3 Complex Workflows ✗
- ✗ **Test**: Multiple wait_for_jobs calls sequentially
- ✗ **Test**: Jobs creating new Jobs dynamically
- ✗ **Test**: Mixed Job and Task usage
- ✗ **Test**: Heterogeneous tuple of Job<T> with different return types

---

## Priority Test Implementation Order

### 1. **Critical** (Correctness)
- **WorkerQueue thread safety** - Per-worker queue isolation, concurrent access
- **Counter synchronization** - Atomic count operations, blocking flag correctness
- **Nested wait_for_jobs** - Thread-local tracking, worker queue stealing
- **Scheduler with varying worker thread counts** - 0, 1, multiple workers
- **Job return values** - std::expected, Job<T>::result() correctness

### 2. **High** (Coverage)
- **wait_for_jobs API variants** - span, tuple, variadic overloads, return value extraction
- **Empty/null edge cases** - Empty spans, no workers, VoidType results
- **Large-scale stress tests** - 1000+ jobs, queue saturation
- **Worker thread distribution** - Round-robin, try_distribute_to_worker logic

### 3. **Medium** (Robustness)
- **Memory leak tests** - JobBase lifecycle, dispatched flag behavior
- **Exception handling** - unhandled_exception in Job and Task
- **Move semantics** - JobBase move constructor/assignment
- **Real-world patterns** - Fan-out, pipeline, parallel execution

### 4. **Low** (Nice-to-have)
- **Performance benchmarks** - Overhead comparison, context switch costs
- **Deep nesting** - 3+ levels of nested wait_for_jobs
- **Very large scale tests** - Extreme job counts, deep call stacks

---

## Test Utilities

### ExecutionTracker
Thread-safe helper for tracking coroutine execution order and verification.

**Location**: `core/tests/jobs_tests.cpp:18-52`

**Methods**:
- `record(coroutine_id)` - Records execution
- `was_executed(coroutine_id)` - Check if executed
- `execution_count()` - Total execution count
- `executed_before(a, b)` - Verify ordering

**Usage**:
```cpp
ExecutionTracker tracker;
jobs.add_job(my_job(tracker));
// After execution:
EXPECT_TRUE(tracker.was_executed("job_1"));
EXPECT_TRUE(tracker.executed_before("job_1", "job_2"));
```

---

## Notes

### Architecture Summary
The job system has undergone significant changes:
- **JobList removed**: Direct use of `std::span<Job>`, `std::tuple`, or variadic jobs
- **Counter struct**: Replaces JobList for tracking completion (`atomic<size_t>` + `atomic_flag`)
- **JobBase + Job<Result>**: Type-erased base class with templated derived for return values
- **std::expected returns**: `Job<T>::result()` returns `std::expected<T, JobResultStatus>`
- **WorkerQueue**: Per-worker `ConcurrentQueue` replaces Channel for better scalability
- **Nested wait_for_jobs**: Thread-local tracking (`tl_current_worker_queue`) enables workers to process own queue during nested waits
- **dispatched flag**: Prevents premature coroutine handle destruction

### Testing Guidelines
- All tests should follow project style: use references instead of pointers where possible
- Use section headers with dividers for organization
- Include clear assertion messages for debugging
- Tests in `core/tests/jobs_tests.cpp` for Job/Scheduler tests
- Tests in `core/tests/tasks_tests.cpp` for Task&lt;T&gt; tests

### Key Files
- `job.h` - JobBase, Job<Result>, JobPromise, ResultPromise, SuspendJob, FinalizeJob
- `job.cpp` - SuspendJob/FinalizeJob implementations, eager worker logic
- `scheduler.h` - Scheduler, Counter, WorkerQueue, WorkerQueueTraits, wait_for_jobs/dispatch_jobs APIs
- `scheduler.cpp` - Scheduler implementation, thread-local tracking, worker_thread_loop
- `task.h` - Task<T> separate system for async continuations