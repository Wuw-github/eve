add_executable(test_log test.cc)
target_link_libraries(test_log sylar)

add_executable(test_config test_config.cc)
target_link_libraries(test_config sylar yaml-cpp)

add_executable(test_thread test_thread.cc)
target_link_libraries(test_thread sylar)