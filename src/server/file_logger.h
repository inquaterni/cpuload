#ifndef CPULOAD_FILE_LOGGER_H
#define CPULOAD_FILE_LOGGER_H

#include <utility>


#include "src/common/static_buffer.h"

class file_logger {
public:
    explicit file_logger(const char *path) noexcept;
    file_logger() noexcept;

    file_logger(const file_logger&) = delete;
    file_logger& operator=(const file_logger&) = delete;

    file_logger(file_logger&& other) noexcept
    : fd(std::exchange(other.fd, -1)) {}
    file_logger& operator=(file_logger&& other) noexcept {
        if (this != &other) {
            if (fd >= 0) {
                close(fd);
            }
            this->fd = other.fd;
        }

        return *this;
    }

    ~file_logger();

    [[nodiscard]]
    constexpr bool enabled() const noexcept { return fd >= 0; }

    void open(const char *path) noexcept;

    void write_snapshot(const static_buffer& data) const noexcept;

private:
    int fd;
};

#endif  // CPULOAD_FILE_LOGGER_H
