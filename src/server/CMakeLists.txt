add_executable(cpuload_server
    main.cpp
    cpu_sampler.cpp
    ipc_server.cpp
    file_logger.cpp
)
target_link_libraries(cpuload_server PRIVATE common)
