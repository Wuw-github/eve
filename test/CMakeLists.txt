add_executable(test_log test.cc)
target_link_libraries(test_log sylar)

add_executable(test_config test_config.cc)
target_link_libraries(test_config sylar yaml-cpp)

add_executable(test_thread test_thread.cc)
target_link_libraries(test_thread sylar)

add_executable(test_util test_util.cc)
target_link_libraries(test_util sylar)

add_executable(test_fiber test_fiber.cc)
target_link_libraries(test_fiber sylar)

add_executable(test_schedule test_schedule.cc)
target_link_libraries(test_schedule sylar)

add_executable(test_iomanager test_iomanager.cc)
target_link_libraries(test_iomanager sylar)