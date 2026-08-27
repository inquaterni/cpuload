#ifndef CPULOAD_FILE_LOGGER_H
#define CPULOAD_FILE_LOGGER_H

#include "src/common/static_buffer.h"

class file_logger {
public:
    explicit file_logger(const char *path) noexcept;

    file_logger(const file_logger&) = delete;
    file_logger& operator=(const file_logger&) = delete;

    ~file_logger();

    [[nodiscard]]
    constexpr bool enabled() const noexcept { return fd >= 0; }

    void write_snapshot(const static_buffer& data) const noexcept;

private:
    int fd;
};

#endif  // CPULOAD_FILE_LOGGER_H
