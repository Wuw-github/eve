set(LOG_SRC_LIST log.cc util.cc)

add_library(sylar SHARED ${LOG_SRC_LIST})

target_link_libraries(sylar pthread)