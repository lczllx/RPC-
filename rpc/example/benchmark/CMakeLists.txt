add_executable(benchmark_server benchmark_server.cc)
target_link_libraries(benchmark_server PRIVATE lcz_rpc)

add_executable(benchmark_client benchmark_client.cc)
target_link_libraries(benchmark_client PRIVATE lcz_rpc)




