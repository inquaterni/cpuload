# cpuload
Test task for GlobalLogic internship. A simple CPU load monitoring tool for Linux.

## Build
Bazel:
```bash
bazel build //src/server:cpuload_server //src/client:cpuload_client
```
CMake:
```bash
cmake -B build && cmake --build build
```

## Usage
Start the server:
```bash
./cpuload_server [-h][--help] [interval_ms] [log_file]
```

- `interval_ms` -- periodic log write interval in milliseconds (default: 1000)
- `log_file` -- path to the log file (default: `/tmp/cpuload`), pass `null` or `none` to disable

Query CPU load:
```bash
./cpuload_client
```

## Design justifications
See [here](JUSTIFICATIONS.md)
