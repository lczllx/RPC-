add_executable(test_server test_server.cc)
target_link_libraries(test_server PRIVATE lcz_rpc)

add_executable(test_client test_client.cc)
target_link_libraries(test_client PRIVATE lcz_rpc)

