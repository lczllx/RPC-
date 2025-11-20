add_executable(test3_server server.cc)
target_link_libraries(test3_server PRIVATE lcz_rpc)

add_executable(publish_client publish_client.cc)
target_link_libraries(publish_client PRIVATE lcz_rpc)

add_executable(subscribe_client subscribe_client.cc)
target_link_libraries(subscribe_client PRIVATE lcz_rpc)

