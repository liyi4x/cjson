cmake_minimum_required(VERSION 3.5)
project (cjson-test)

# include_directories(
#     ${PROJECT_SOURCE_DIR}/include
# )

add_library(cjson SHARED
    src/cjson.c
)
target_include_directories(cjson PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_library(cjson::library ALIAS cjson)



add_executable(cjson-test
    test/test.c
)

target_link_libraries(cjson-test
    cjson::library
)
