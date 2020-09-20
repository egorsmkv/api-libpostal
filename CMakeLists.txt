cmake_minimum_required(VERSION 3.1)
project(api_libpostal C)
set(CMAKE_C_STANDARD 11)

add_executable(api_libpostal main.c responses.h httpserver.h log.h log.c sdsalloc.h sds.h sds.c uri_encode.c uri_encode.h parson.c parson.h)

target_link_libraries(api_libpostal postal)
