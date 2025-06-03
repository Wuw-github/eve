set(LOG_SRC_LIST log.cc util.cc config.cc config_log.cc 
                    thread.cc mutex.cc fiber.cc scheduler.cc
                    iomanager.cc)

add_library(sylar SHARED ${LOG_SRC_LIST})

target_link_libraries(sylar pthread yaml-cpp)