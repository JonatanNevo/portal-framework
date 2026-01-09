# Core Module

```{Warning}
This section is still under construction, everything is subject to change and might be out of date!
```


The foundation layer providing utilities used by all other modules.

## Purpose

Core contains platform-agnostic utilities: logging, assertions,
memory utilities, math types, and the job system.

### Key Components

- **Logging** — Structured logging with levels and categories
- **Jobs** — Coroutine-based job system for parallel work
- **Math** — GLM-based math types and utilities
- **Memory** — Custom allocators and smart pointer utilities

### Dependencies

None. Core is the base layer.

### Used By

All other Portal modules depend on core.

### Quick Example

```c++
#include <portal/core/log.hpp>
#include <portal/core/jobs/job_system.hpp>

portal::log::info("Hello from Portal!");
```

### API Reference
