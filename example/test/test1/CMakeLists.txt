add_executable(registry_server registry_server.cc)
target_link_libraries(registry_server PRIVATE lcz_rpc)

add_executable(rpc_server rpc_server.cc)
target_link_libraries(rpc_server PRIVATE lcz_rpc)

add_executable(rpc_client rpc_client.cc)
target_link_libraries(rpc_client PRIVATE lcz_rpc)

