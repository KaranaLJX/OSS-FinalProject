cmake_minimum_required(VERSION 3.5)

project(network_test)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0 -Wall -Wextra -Wpedantic -DDEBUG" CACHE STRING "" FORCE)

set(USE_JSON OFF CACHE BOOL "output in json format" FORCE)

if (USE_JSON)
add_compile_definitions(USE_JSON)
endif()


aux_source_directory(${PROJECT_SOURCE_DIR}/src src_files)
set(SRC ${src_files})

add_executable(network_test ${SRC})

set_property(TARGET network_test PROPERTY CXX_STANDARD 17)

target_include_directories(network_test PRIVATE ${PROJECT_SOURCE_DIR}/Include)

if (USE_JSON)
target_include_directories(network_test PRIVATE ${PROJECT_SOURCE_DIR}/third_party/rapidjson/)
endif()


target_link_libraries(network_test PRIVATE ${PROJECT_SOURCE_DIR}/Lib/x64/libws2_32.a ${PROJECT_SOURCE_DIR}/Lib/x64/wpcap.lib ${PROJECT_SOURCE_DIR}/Lib/x64/Packet.lib ${PROJECT_SOURCE_DIR}/Lib/x64/libimagehlp.a)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(network_test PRIVATE -static-libgcc -static-libstdc++)
endif()
