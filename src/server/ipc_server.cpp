
#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "ipc_server.h"
#include "src/common/protocol.h"
#include "src/common/static_buffer.h"


ipc_server::ipc_server() noexcept : listen_fd(-1) {
    unlink(SOCKET_PATH);

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", SOCKET_PATH);

    if (bind(listen_fd,
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) < 0) {
        close(listen_fd);
        listen_fd = -1;
        return;
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        close(listen_fd);
        unlink(SOCKET_PATH);
        listen_fd = -1;
    }
}

ipc_server::~ipc_server() {
    if (listen_fd >= 0) {
        close(listen_fd);
        unlink(SOCKET_PATH);
    }
}

void ipc_server::handle_client(const static_buffer& response) const noexcept {
    const int client_fd = accept(listen_fd, nullptr, nullptr);
    if (client_fd < 0) return;

    uint16_t cmd = 0;
    if (const auto n = read(client_fd, &cmd, sizeof(cmd));
        n == static_cast<ssize_t>(sizeof(cmd)) && cmd == MAGIC) {

        const char* ptr = response.data();
        std::size_t rem = response.length();
        while (rem > 0) {
            const ssize_t w = write(client_fd, ptr, rem);
            if (w < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (w == 0) break;
            ptr += w;
            rem -= static_cast<std::size_t>(w);
        }
    }

    close(client_fd);
}
