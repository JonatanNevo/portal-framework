# Job System Test Plan

Comprehensive test plan for the Portal Framework job system covering all features and edge cases.

## Test Status Legend
- ✓ = Implemented
- ⧗ = Partially implemented
- ✗ = Not yet implemented

---

## 1. **Job Basics**

### 1.1 Single Job Execution ✓
- ✓ **Test**: Single job completes successfully
- ✓ **Test**: Job returns void properly
- ✓ **Test**: Job can be created and destroyed without execution

### 1.2 SuspendJob Awaitable ✗
- ✗ **Test**: Job suspends and resumes correctly
- ✗ **Test**: Multiple suspensions in single job
- ✗ **Test**: Suspension at different points in job execution

### 1.3 FinalizeJob ✗
- ✗ **Test**: final_suspend decrements job count
- ✗ **Test**: Job handle is destroyed after finalization
- ✗ **Test**: JobList unblocks when last job finalizes

---

## 2. **JobList**

### 2.1 Basic Operations ✗
- ✗ **Test**: Add single job to list
- ✗ **Test**: Add multiple jobs to list
- ✗ **Test**: get_job_count() returns correct count
- ✗ **Test**: JobList with custom capacity_hint

### 2.2 Job Count Management ✗
- ✗ **Test**: Job count increments on add_job
- ✗ **Test**: Job count decrements on job completion
- ✗ **Test**: Job count reaches zero after all jobs complete

### 2.3 Concurrent Access ✗
- ✗ **Test**: Multiple threads adding jobs concurrently (if allowed by design)
- ✗ **Test**: Pop job from multiple threads simultaneously
- ✗ **Test**: Push suspended jobs from multiple threads

### 2.4 Blocking/Unblocking ✗
- ✗ **Test**: register_blocking() prevents multiple threads from blocking
- ✗ **Test**: unblock() wakes blocked thread
- ✗ **Test**: wait_until_end() blocks until job count reaches zero
- ✗ **Test**: Unblock called when job count reaches zero

### 2.5 Destructor Behavior ✗
- ✗ **Test**: Destructor destroys all remaining jobs
- ✗ **Test**: JobList destroyed with pending jobs

---

## 3. **Scheduler**

### 3.1 Creation & Configuration ⧗
- ✓ **Test**: Create scheduler with 0 worker threads (main thread only)
- ✓ **Test**: Create scheduler with 1 worker thread
- ✗ **Test**: Create scheduler with multiple worker threads
- ✗ **Test**: Create scheduler with -1 (hardware_concurrency - 1)
- ✗ **Test**: Scheduler with more threads than hardware cores

### 3.2 Job Distribution ✗
- ✗ **Test**: Jobs distributed across worker threads
- ✗ **Test**: Job executed on main thread when all workers busy
- ✗ **Test**: Single job on single-threaded scheduler
- ✗ **Test**: More jobs than worker threads

### 3.3 wait_for_job_list ⧗
- ✓ **Test**: Returns when all jobs complete
- ✗ **Test**: Blocks until completion with pending jobs
- ✗ **Test**: Handles empty JobList
- ✗ **Test**: Multiple consecutive wait_for_job_list calls
- ✗ **Test**: Thread that calls wait_for_job_list executes jobs

### 3.4 Destructor & Cleanup ✗
- ✗ **Test**: Destructor unlocks all channels
- ✗ **Test**: Worker threads stop gracefully
- ✗ **Test**: Scheduler destruction with pending jobs

---

## 4. **Channel**

### 4.1 Basic Operations ✗
- ✗ **Test**: try_push succeeds on empty channel
- ✗ **Test**: try_push fails on occupied channel
- ✗ **Test**: get_payload returns correct handle
- ✗ **Test**: clear() makes channel available again

### 4.2 Wait/Notify ✗
- ✗ **Test**: wait() blocks until payload available
- ✗ **Test**: Worker thread wakes on try_push
- ✗ **Test**: unlock() wakes waiting thread (for shutdown)
- ✗ **Test**: Spurious wakeup handling

### 4.3 Move Semantics ✗
- ✗ **Test**: Channel move constructor transfers payload
- ✗ **Test**: Channel move assignment transfers payload
- ✗ **Test**: Move from channel with payload
- ✗ **Test**: Move from empty channel

### 4.4 Destructor ✗
- ✗ **Test**: Destructor cleans up leftover handle
- ✗ **Test**: Destructor with empty channel

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
- ✗ **Test**: Concurrent job_count modifications
- ✗ **Test**: Concurrent push/pop on JobList
- ✗ **Test**: Concurrent channel access
- ✗ **Test**: Multiple threads calling wait_for_job_list

### 6.3 Thread Safety ✗
- ✗ **Test**: Ring buffer thread safety
- ✗ **Test**: Atomic operations correctness
- ✗ **Test**: Block flag memory ordering

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
- ✗ **Test**: Empty JobList passed to wait_for_job_list
- ✗ **Test**: pop_job returns nullptr when empty
- ✗ **Test**: Scheduler with no channels/threads

### 8.2 Large Scale ✗
- ✗ **Test**: 1000+ jobs in single JobList
- ✗ **Test**: Very deep call stacks
- ✗ **Test**: Large capacity_hint values

### 8.3 Stress Tests ✗
- ✗ **Test**: Rapid job creation/completion cycles
- ✗ **Test**: Long-running jobs mixed with short jobs
- ✗ **Test**: All threads suspended simultaneously

### 8.4 Memory & Lifetime ✗
- ✗ **Test**: No memory leaks with many jobs
- ✗ **Test**: Coroutine handle lifetime management
- ✗ **Test**: JobList consumed by wait_for_job_list (scheduler.h:48 comment)

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
- ✗ **Test**: Multiple JobLists executed sequentially
- ✗ **Test**: Jobs creating new Jobs dynamically
- ✗ **Test**: Mixed Job and Task usage

---

## Priority Test Implementation Order

### 1. **Critical** (Correctness)
- Channel thread safety tests
- Job count synchronization
- Nested job edge cases
- Scheduler with varying worker thread counts

### 2. **High** (Coverage)
- Empty/null edge cases
- Large-scale stress tests
- Task&lt;T&gt; return value handling
- Worker thread distribution

### 3. **Medium** (Robustness)
- Memory leak tests
- Exception handling
- Move semantics
- Real-world patterns

### 4. **Low** (Nice-to-have)
- Performance benchmarks
- Deep nesting (3+ levels)
- Very large scale tests

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

- All tests should follow project style: use references instead of pointers where possible
- Use section headers with dividers for organization
- Include clear assertion messages for debugging
- Tests in `core/tests/jobs_tests.cpp` for Job/Scheduler tests
- Tests in `core/tests/tasks_tests.cpp` for Task&lt;T&gt; tests