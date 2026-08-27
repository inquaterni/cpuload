#include <chrono>
#include <fcntl.h>
#include <unistd.h>

#include "file_logger.h"

file_logger::file_logger(const char *path) noexcept : fd(-1) {
    if (!path)
        return;
    tzset();
    fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
}

file_logger::~file_logger() {
    if (fd >= 0) {
        close(fd);
    }
}

void file_logger::write_snapshot(const static_buffer &data) const noexcept {
    if (fd < 0)
        return;

    char ts_buf[64];
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_t = std::chrono::system_clock::to_time_t(now);
    tm tm_buf{};
    localtime_r(&now_t, &tm_buf);

    const int ts_len = std::snprintf(ts_buf, sizeof(ts_buf), "[%04d-%02d-%02d %02d:%02d:%02d]\n", tm_buf.tm_year + 1900,
                                     tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);

    if (ts_len > 0) {
        write(fd, ts_buf, static_cast<std::size_t>(ts_len));
    }
    write(fd, data.data(), data.length());
    write(fd, "\n", 1);
}
