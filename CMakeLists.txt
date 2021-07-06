cmake_minimum_required(VERSION 2.6)
project (cjson-test C)

add_library(cjson
    cjson.c
)

add_executable(cjson-test
    test.c
)

target_link_libraries(cjson-test
    cjson
)
