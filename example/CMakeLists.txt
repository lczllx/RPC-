add_executable(message_test message_test.cc)
target_link_libraries(message_test PRIVATE lcz_rpc)

add_subdirectory(test)

