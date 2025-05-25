
set(SRC_LIST test.cc)

add_executable(test_log ${SRC_LIST})
target_link_libraries(test_log sylar)