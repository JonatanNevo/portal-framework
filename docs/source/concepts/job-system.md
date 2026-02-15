# Job System

Portal Framework provides a task-based cooperative multitasking system built on C++20 coroutines.
This architecture allows for efficient utilization of multi-core processors while providing a high-level, asynchronous programming model.

## Architecture Overview

The job system is designed around a work-stealing scheduler that manages a fixed pool of worker threads.

```{mermaid}
flowchart TB
    subgraph arch[Job System Architecture]
        direction TB
        Scheduler[Scheduler]

        subgraph Workers[Worker Pool]
            W1[Worker 1<br/>Local Queue + Cache]
            W2[Worker 2<br/>Local Queue + Cache]
            WN[Worker N<br/>Local Queue + Cache]
        end

        GlobalQueue[Global Queue]

        Scheduler --> Workers
        GlobalQueue -.-> Workers
    end

    subgraph ops[Worker Operations Priority]
        direction TB
        A[1. Pop from own Local Queue] --> B[2. Steal from other Workers]
        B --> C[3. Dequeue from Global Queue]
    end
```

### Key Principles

1. **Non-Blocking**: Jobs are coroutines. When a job needs to wait for another job, it suspends itself, allowing the
   worker thread to pick up other work.
2. **Work Stealing**: To prevent load imbalance, idle worker threads "steal" jobs from other workers' queues.
3. **Prioritization**: The scheduler supports different priority levels (Low, Normal, High) to ensure critical tasks are
   handled first.
4. **Locality**: Workers prioritize their own local queue and a small job cache to improve cache hits and reduce
   contention.

## Core Components

### Job

A [portal::Job](exhale_class_classportal_1_1Job) is the primary unit of work. It is a coroutine that can return a value
of type `T` (or `void`).

- **Managed Lifecycle**: Jobs are owned by the scheduler once dispatched.
- **Awaitable**: You can `co_await` a job to suspend the current coroutine until the awaited job completes. The result of `co_await` is a `std::expected<T, JobResultStatus>`, allowing for robust error propagation.
- **Result Access**: When using `wait_for_job` instead, the result is unwrapped and returned as `T` directly.

### Scheduler

The [portal::jobs::Scheduler](exhale_class_classportal_1_1jobs_1_1Scheduler) is the heart of the system.
It initializes worker threads and manages the distribution of work.

- **Worker Threads**: By default, the scheduler spawns one worker thread per hardware core (minus one for the main thread). This can be configured via the constructor.
- **Dispatching**: Use `dispatch_job` to submit a single job, or `dispatch_jobs` to submit a batch. Both are fire-and-forget; they return immediately without waiting for completion.
- **Waiting**: Use `wait_for_job` or `wait_for_counter` to wait until work is complete. While waiting, the calling thread continues to process other available jobs rather than blocking idle.
- **Main Thread Participation**: The `main_thread_do_work()` method allows the main thread to participate in job execution, processing available work from the global queue.

### Counter

A [portal::jobs::Counter](exhale_struct_structportal_1_1jobs_1_1Counter) is a synchronization primitive used to track
the completion of multiple jobs.

- **Batching**: You can dispatch multiple jobs and associate them with a single counter by passing it to `dispatch_job` or `dispatch_jobs`.
- **Synchronization**: Use `scheduler.wait_for_counter(counter)` to wait until all associated jobs have finished.

## Execution Flow

1. **Dispatch**: A job is created and passed to `Scheduler::dispatch_job`.
2. **Queueing**: The job is placed in a local queue (if dispatched from a worker) or the global queue.
3. **Acquisition**: A worker thread pulls the job from its local queue, steals it from another worker, or fetches it
   from the global queue.
4. **Execution**: The worker thread resumes the job coroutine.
5. **Suspension**: If the job `co_await`s another job or a counter, it suspends, and the worker moves to the next
   available task. A job can also voluntarily yield by using `co_await SuspendJob()`, which re-queues the job and returns control to the worker thread.
6. **Completion**: When the coroutine reaches its end, it notifies any waiting continuations or counters.

## Thread Safety Guarantees

- **Scheduler API**: Most scheduler methods (`dispatch_job`, `wait_for_job`) are thread-safe and can be called from any
  thread.
- **Worker Queues**: The local queues use lock-free or low-contention designs to minimize overhead during work stealing.
  Each worker maintains separate local and stealable queue sets to reduce contention during normal operation.
- **Data Access**: While the job system manages *execution*, it does not automatically synchronize access to shared
  data. Developers must use standard synchronization primitives (mutexes, atomics) or ensure data is uniquely owned by a
  job.

## Best Practices

- **Avoid Long-Running Blocking Calls**: Since worker threads are a limited resource, avoid `std::mutex::lock()` or
  other blocking I/O within a job. Use async alternatives or suspend the job.
- **Grain Size**: Jobs should be large enough to justify the scheduling overhead but small enough to allow for effective
  parallelization.
- **Prefer co_await**: Within a job, always use `co_await other_job` instead of `wait_for_job` to allow the worker
  thread to remain productive.
- **Use Counters for Batch Work**: When dispatching many independent jobs, associate them with a single `Counter` and wait on it, rather than waiting on each job individually.
