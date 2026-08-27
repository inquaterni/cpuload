#ifndef CPULOAD_PROTOCOL_H
#define CPULOAD_PROTOCOL_H

#include <cstddef>
#include <cstdint>

inline constexpr auto SOCKET_PATH = "/tmp/cpuload.sock";

inline constexpr int MAX_CORES = 256;

inline constexpr uint16_t MAGIC = 0x0C92;

inline constexpr std::size_t MAX_RESPONSE_SIZE = 8192;

inline constexpr int BACKLOG = 4;

#endif  // CPULOAD_PROTOCOL_H
