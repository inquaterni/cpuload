#ifndef CPULOAD_FIXED_BUFFER_H
#define CPULOAD_FIXED_BUFFER_H
#include <cstdarg>
#include <cstddef>
#include <cstdio>

class static_buffer {
public:
    static constexpr std::size_t capacity = 8192;

    constexpr static_buffer() noexcept = default;

    static_buffer(const static_buffer&) = delete;
    static_buffer& operator=(const static_buffer&) = delete;

    ~static_buffer() noexcept = default;

    constexpr void clear() noexcept {
        size = 0;
        buf[0] = '\0';
    }

    [[nodiscard]]
    constexpr const char* data() const noexcept { return buf; }

    [[nodiscard]]
    constexpr std::size_t length() const noexcept { return size; }

    [[nodiscard]]
    constexpr std::size_t remaining() const noexcept {
        return size < capacity - 1 ? capacity - size - 1 : 0;
    }

    std::size_t appendf(const char* fmt, ...) noexcept
        __attribute__((format(printf, 2, 3)))
    {
        if (size >= capacity - 1) return 0;

        std::va_list args;
        va_start(args, fmt);
        const int n = vsnprintf(buf + size, capacity - size, fmt, args);
        va_end(args);

        if (n > 0) {
            auto written = static_cast<std::size_t>(n);
            if (const std::size_t avail = capacity - size - 1; written > avail)
                written = avail;
            size += written;
        }
        return n > 0 ? static_cast<std::size_t>(n) : 0;
    }

    constexpr std::size_t append(const char* src, const std::size_t len) noexcept {
        if (size >= capacity - 1) return 0;
        const std::size_t avail = capacity - size - 1;
        const std::size_t n = len < avail ? len : avail;
        for (std::size_t i = 0; i < n; ++i)
            buf[size + i] = src[i];
        size += n;
        buf[size] = '\0';
        return n;
    }

private:
    char buf[capacity]{};
    std::size_t size{};
};

#endif  // CPULOAD_FIXED_BUFFER_H
