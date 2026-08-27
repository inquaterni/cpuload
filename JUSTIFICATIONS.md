# Design Decisions and Justifications

Here are the main design choices I made while building this CPU load monitoring app.

## Server-client architecture over Unix domain sockets

I split the app into a background server and a client tool, your CLI, since app that constantly in the foreground
is annoying. IPC a Unix domain socket at /tmp/cpuload.sock. 

I thought about using POSIX pipes since they have a simpler API, but they are only one-way.
I'd need two pipes per connection and extra bookkeeping to handle multiple clients. I'm more familiar with sockets,
which give clean, request-response behavior in a single file descriptor, and the kernel handles moving data between
the processes with basically zero overhead. I went with SOCK_STREAM because it naturally fits the lifecycle of
connecting, sending the magic command, getting the data, and disconnecting, and it's simpler.

## Init vs Run States

I didn't bother making the Init and Run states explicit in the code. Basically all programs work like that, so there's 
literally no reason to add an actual state machine for it. It's just too simple to be explicit.

## /proc/stat for CPU load data

The server reads the raw CPU counters directly from /proc/stat. There isn't a standard POSIX API for getting per-core
utilization, so parsing /proc/stat is really the only viable way to do it on Linux.

## Minimal user-space heap allocation

I decided to write the entire user-space side without any explicit heap allocation at all. It didn't eliminate all
heap usage though.

But practically speaking, the stack footprint is tiny and very predictable:
- cpu_sampler previous stats: ~16 KiB (max 256 cores)
- cpu_sampler current stats: ~16 KiB
- The buffer for reading /proc/stat: 64 KiB
- Our custom fixed_buffer for formatting: ~8 KiB

Total peak stack usage is around 100 KiB, which is well within normal thread limits.

## poll() event multiplexing

The server's main loop uses poll() instead of something more complex. It's perfectly fine for the concurrency model 
I implemented and keeps the code straightforward.