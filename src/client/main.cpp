#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "src/common/protocol.h"

int main() {
    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "Failed to open a socket." << std::endl;
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", SOCKET_PATH);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Failed to connect to a server. Is server running?" << std::endl;
        close(fd);
        return 1;
    }

    constexpr uint16_t cmd = MAGIC;
    if (write(fd, &cmd, sizeof(cmd)) != static_cast<ssize_t>(sizeof(cmd))) {
        std::cerr << "Failed to send stat request." << std::endl;
        close(fd);
        return 1;
    }

    char buf[MAX_RESPONSE_SIZE];
    ssize_t total = 0;
    while (total < static_cast<ssize_t>(sizeof(buf) - 1)) {
        const ssize_t n = read(fd, buf + total,
                               sizeof(buf) - 1 - static_cast<std::size_t>(total));
        if (n <= 0) break;
        total += n;
    }

    if (total > 0) {
        write(STDOUT_FILENO, buf, static_cast<std::size_t>(total));
    }

    close(fd);
    return 0;
}
