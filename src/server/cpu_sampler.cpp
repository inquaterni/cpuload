#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "cpu_sampler.h"

static uint64_t next_u64(const char*& ptr) noexcept {
    while (*ptr == ' ' || *ptr == '\t') ++ptr;

    uint64_t val = 0;
    bool found = false;
    while (*ptr >= '0' && *ptr <= '9') {
        val = val * 10 + static_cast<uint64_t>(*ptr - '0');
        ++ptr;
        found = true;
    }
    return found ? val : 0;
}

static void parse_cpu_fields(const char* p, cpu_times& t) noexcept {
    t.user    = next_u64(p);
    t.nice    = next_u64(p);
    t.system  = next_u64(p);
    t.idle    = next_u64(p);
    t.iowait  = next_u64(p);
    t.irq     = next_u64(p);
    t.softirq = next_u64(p);
    t.steal   = next_u64(p);
}

bool cpu_sampler::sample() noexcept {
    prev = curr;
    const int cores = read_proc_stat(curr);
    if (cores < 0) return false;
    num_cores = static_cast<std::size_t>(cores);
    return true;
}

void cpu_sampler::format_all(static_buffer& buf) const noexcept {
    buf.clear();
    buf.appendf("CPU Load (%zu cores)\n", num_cores);
    buf.appendf("-------------------\n");

    const int overall = load_percent(0);
    buf.appendf("Overall : %3d%%\n", overall);

    for (std::size_t i = 1; i <= num_cores; ++i) {
        const int load = load_percent(i);
        buf.appendf("Core %3zu: %3d%%\n", i - 1, load);
    }
}

int cpu_sampler::read_proc_stat(
        std::array<cpu_times, MAX_CORES + 1>& out) noexcept {
    const int fd = open("/proc/stat", O_RDONLY);
    if (fd < 0) return -1;

    char buf[buf_size];
    ssize_t bytes_read = 0;

    while (bytes_read < static_cast<ssize_t>(buf_size - 1)) {
        const ssize_t n = read(fd, buf + bytes_read,
                               buf_size - 1 - static_cast<std::size_t>(bytes_read));
        if (n <= 0) break;
        bytes_read += n;
    }
    close(fd);
    buf[bytes_read] = '\0';

    int core_count = 0;
    char* line = buf;

    while (line != nullptr && *line != '\0') {
        char* next = std::strchr(line, '\n');
        if (next != nullptr) *next = '\0';

        cpu_times t{};

        if (std::strncmp(line, "cpu ", 4) == 0) {
            parse_cpu_fields(line + 4, t);
            out[0] = t;
        } else if (std::strncmp(line, "cpu", 3) == 0 &&
                   line[3] >= '0' && line[3] <= '9') {
            int core_id = 0;
            const char* p = line + 3;
            while (*p >= '0' && *p <= '9') {
                core_id = core_id * 10 + (*p - '0');
                ++p;
            }

            if (core_id < MAX_CORES) {
                parse_cpu_fields(p, t);
                out[core_id + 1] = t;
                if (core_id + 1 > core_count)
                    core_count = core_id + 1;
            }
        } else {
            break;
        }

        line = next != nullptr ? next + 1 : nullptr;
    }

    return core_count;
}
