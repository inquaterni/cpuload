#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <poll.h>

#include "cpu_sampler.h"
#include "file_logger.h"
#include "ipc_server.h"

namespace {

    using steady_clock = std::chrono::steady_clock;
    using milliseconds = std::chrono::milliseconds;

    int64_t now_ms() noexcept {
        return std::chrono::duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    void print_usage(const char* argv[]) noexcept {
        std::fprintf(stderr,
            "Usage: %s [interval_milliseconds] [log_file]\n\n"
            "Parameters:\n"
            "interval_seconds\tPeriodic file-write interval\n"
            "log_file\t\tPath to the log file\n\n",
            argv[0]);
    }

}

int main(const int argc, const char* argv[]) {

    long write_interval_ms = 1000;
    auto log_file  = "/tmp/cpuload";

    if (argc == 3) {
        char* end = nullptr;
        const auto val = std::strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || val == 0) {
            std::fprintf(stderr, "error: interval must be a positive integer, got %s\n", argv[1]);
            print_usage(argv);
            return 1;
        }
        write_interval_ms = val;
        if (strcmp(argv[2], "null") != 0 && strcmp(argv[2], "none") != 0) {
            log_file = argv[2];
        } else {
            log_file = nullptr;
        }
    } else if (argc == 2) {
        if (const char *arg = argv[1]; strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_usage(argv);
        } else {
            char *end = nullptr;
            if (const auto val = std::strtol(arg, &end, 10); val == 0) {
                if (end == argv[1] || *end != '\0' || val == 0) {
                    std::fprintf(stderr, "error: interval must be a positive integer, got %s\n", argv[1]);
                    print_usage(argv);
                    return 1;
                }
            }
            std::fprintf(stderr, "Unknown option: %s\n", arg);
            print_usage(argv);
        }
        return 1;
    }

    cpu_sampler sampler;
    std::fprintf(stderr, "Detected %zu CPU core(s)\n",
                 sampler.hardware_concurrency());

    const ipc_server server;
    if (!server.valid()) {
        std::fprintf(stderr, "error: failed to create socket %s: %s\n",
                     SOCKET_PATH, std::strerror(errno));
        return 1;
    }
    std::fprintf(stderr, "Listening on %s...\n", SOCKET_PATH);

    if (log_file != nullptr) {
        std::fprintf(stderr, "Logging to %s every %lu ms\n",
                     log_file, write_interval_ms);
    }
    const file_logger logger(log_file);
    if (log_file != nullptr && !logger.enabled()) {
        std::fprintf(stderr, "error: failed to open log file %s: %s\n",
                     log_file, std::strerror(errno));
        return 1;
    }

    const int64_t interval_ms = write_interval_ms;
    int64_t last_write_ms = now_ms();

    pollfd pfd{};
    pfd.fd     = server.listen_fd();
    pfd.events = POLLIN;
    static_buffer buf;

    while (true) {
        int timeout_ms {-1};
        if (logger.enabled()) {
            const int64_t elapsed = now_ms() - last_write_ms;
            const int64_t remain  = interval_ms - elapsed;
            if (remain >= std::numeric_limits<int>::max()) {
                std::fprintf(stderr, "warn: interval exceeded the numeric limits of the integer type");
            }
            timeout_ms = remain > 0 ? static_cast<int>(remain) : 0;
        }

        const int ret = poll(&pfd, 1, timeout_ms);
        const bool client_pending = ret > 0 && (pfd.revents & POLLIN) != 0;
        const bool file_due =
            logger.enabled() && now_ms() - last_write_ms >= interval_ms;

        if (client_pending || file_due) {
            sampler.sample();
            sampler.format_all(buf);

            if (client_pending) {
                server.handle_client(buf);
            }

            if (file_due) {
                logger.write_snapshot(buf);
                last_write_ms = now_ms();
            }
        }
    }
}
