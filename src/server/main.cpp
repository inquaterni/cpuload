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

    enum class state: uint8_t {
        INIT,
        RUN
    };

    struct context {
        // A lot
        cpu_sampler      sampler;
        static_buffer    buf;
        // 8
        long             write_interval_ms = 1000;
        const char*      log_file          = "/tmp/cpuload";
        pollfd           fds {};
        int64_t          last_write_ms {-1};
        // 4
        const ipc_server server;
        file_logger      logger;
    };

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

    int parse_cli(const int argc, const char* argv[], context &ctx) noexcept {
        if (argc == 3) {
            char* end = nullptr;
            const auto val = std::strtol(argv[1], &end, 10);
            if (end == argv[1] || *end != '\0' || val == 0) {
                std::fprintf(stderr, "error: interval must be a positive integer, got %s\n", argv[1]);
                print_usage(argv);
                return 1;
            }
            ctx.write_interval_ms = val;
            if (strcmp(argv[2], "null") != 0 && strcmp(argv[2], "none") != 0) {
                ctx.log_file = argv[2];
            } else {
                ctx.log_file = nullptr;
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
        return 0;
    }

}

int main(const int argc, const char* argv[]) {

    auto s = state::INIT;
    context ctx;
    int timeout_ms {-1};

    while (true) {
        switch (s) {
            case state::INIT: {
                if (const auto val = parse_cli(argc, argv, ctx); val != 0) {
                    return val;
                }
                std::fprintf(stderr, "Detected %zu CPU core(s)\n",
                     ctx.sampler.hardware_concurrency());

                // initial sample
                ctx.sampler.sample();

                if (!ctx.server.valid()) {
                    std::fprintf(stderr, "error: failed to create socket %s: %s\n",
                                 SOCKET_PATH, std::strerror(errno));
                    return 1;
                }

                if (ctx.log_file != nullptr) {
                    ctx.logger.open(ctx.log_file);
                    std::fprintf(stderr, "Logging to %s every %lu ms\n",
                                 ctx.log_file, ctx.write_interval_ms);
                    if (!ctx.logger.enabled()) {
                        std::fprintf(stderr, "error: failed to open log file %s: %s\n",
                                     ctx.log_file, std::strerror(errno));
                        return 1;
                    }
                }

                ctx.fds.fd     = ctx.server.listen_fd();
                ctx.fds.events = POLLIN;

                s = state::RUN;
            } break;
            case state::RUN: {
                if (ctx.logger.enabled()) {
                    const int64_t elapsed = now_ms() - ctx.last_write_ms;
                    const int64_t remain  = ctx.write_interval_ms - elapsed;
                    if (remain >= std::numeric_limits<int>::max()) {
                        std::fprintf(stderr, "warn: interval exceeded the numeric limits of the integer type");
                    }
                    timeout_ms = remain > 0 ? static_cast<int>(remain) : 0;
                }

                const int ret = poll(&ctx.fds, 1, timeout_ms);
                const bool client_pending = ret > 0 && (ctx.fds.revents & POLLIN) != 0;

                if (const bool file_due = ctx.logger.enabled() && now_ms() - ctx.last_write_ms >= ctx.write_interval_ms;
                    client_pending || file_due) {
                    ctx.sampler.sample();
                    ctx.sampler.format_all(ctx.buf);

                    if (client_pending) {
                        ctx.server.handle_client(ctx.buf);
                    }

                    if (file_due) {
                        ctx.logger.write_snapshot(ctx.buf);
                        ctx.last_write_ms = now_ms();
                    }
                }
            } break;
        }
    }
}
