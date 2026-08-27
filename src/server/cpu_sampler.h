#ifndef CPULOAD_CPU_SAMPLER_H
#define CPULOAD_CPU_SAMPLER_H
#include <array>
#include <thread>

#include "src/common/static_buffer.h"
#include "src/common/protocol.h"

struct cpu_times {
    uint64_t user    = 0;
    uint64_t nice    = 0;
    uint64_t system  = 0;
    uint64_t idle    = 0;
    uint64_t iowait  = 0;
    uint64_t irq     = 0;
    uint64_t softirq = 0;
    uint64_t steal   = 0;

    [[nodiscard]]
    constexpr uint64_t total() const noexcept {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    [[nodiscard]]
    constexpr uint64_t active() const noexcept {
        return total() - idle - iowait;
    }
};

class cpu_sampler {
public:
    cpu_sampler() noexcept
        : num_cores(std::thread::hardware_concurrency()) {}

    cpu_sampler(const cpu_sampler&) = delete;
    cpu_sampler& operator=(const cpu_sampler&) = delete;

    bool sample() noexcept;

    [[nodiscard]]
    constexpr std::size_t hardware_concurrency() const noexcept {
        return num_cores;
    }

    [[nodiscard]]
    constexpr int load_percent(std::size_t index) const noexcept;

    void format_all(static_buffer& buf) const noexcept;

private:
    constexpr static std::size_t buf_size = 1 << 16;
    std::array<cpu_times, MAX_CORES + 1> prev{};
    std::array<cpu_times, MAX_CORES + 1> curr{};
    std::size_t num_cores;

    static int read_proc_stat(std::array<cpu_times, MAX_CORES + 1>& out) noexcept;
};

constexpr int cpu_sampler::load_percent(const std::size_t index) const noexcept {
    if (index > num_cores) return -1;

    const auto& p = prev[index];
    const auto& c = curr[index];

    const uint64_t total_delta = c.total() - p.total();
    if (total_delta == 0) return 0;

    const uint64_t active_delta = c.active() - p.active();
    return static_cast<int>(active_delta * 100 / total_delta);
}

#endif  // CPULOAD_CPU_SAMPLER_H
