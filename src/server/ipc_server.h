#ifndef CPULOAD_IPC_SERVER_H
#define CPULOAD_IPC_SERVER_H

class static_buffer;

class ipc_server {
public:
    ipc_server() noexcept;

    ipc_server(const ipc_server&) = delete;
    ipc_server& operator=(const ipc_server&) = delete;

    ~ipc_server();

    [[nodiscard]]
    constexpr bool valid() const noexcept { return listen_fd >= 0; }

    [[nodiscard]]
    constexpr int fd() const noexcept { return listen_fd; }

    void handle_client(const static_buffer& response) const noexcept;

private:
    int listen_fd;
};

#endif  // CPULOAD_IPC_SERVER_H
