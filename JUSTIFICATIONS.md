# Design Decisions and Justifications
## Server-client architecture

The application is split into a long-running server and a one-shot client, communicating over a
`SOCK_STREAM` Unix domain socket at `/tmp/cpuload.sock`.

POSIX pipes were considered as an alternative IPC mechanism, but they are unidirectional by design.
Supporting bidirectional communication would require two pipes per connection along with additional bookkeeping
for multi-client scenarios. Unix domain sockets provide clean request-response semantics in a single file
descriptor with minimal kernel overhead. `SOCK_STREAM` was chosen over `SOCK_DGRAM` because it maps directly
to the connect-send-receive-close lifecycle of each client query, and it's API is simpler.

## /proc/stat for CPU load data

The server reads cumulative per-core CPU counters from `/proc/stat` and computes load as the percentage delta between
two consecutive samples. There is no standard POSIX API that provides per-core CPU utilization breakdowns, making
`/proc/stat` the only viable data source on Linux.

## Minimal user-space heap allocation

All user-space data structures use stack or statically-sized storage. The binary is compiled with `-fno-exceptions`
and `-fno-rtti`.

Residual heap usage comes exclusively from shared library internals:
- `eh_alloc.cc`: 72 KiB, allocated by the dynamic linker before `main()`
- timezone cache (`tzset`/`__tzfile_read`): ~1.4 KiB, allocated during Init via `file_logger` constructor

### Stack footprint

| Component                     | Size        |
|-------------------------------|-------------|
| `cpu_sampler::prev`           | 16.0625 KiB |
| `cpu_sampler::curr`           | 16.0625 KiB |
| `read_proc_stat` local buffer | 64 KiB      |
| `static_buffer`               | 8.008 KiB   |

Peak stack usage during sampling: 104.13 KiB, well within the default thread stack limit.
Confirmed by Massif: idle stack ~41 KiB, peak ~107 KiB (including call frames).