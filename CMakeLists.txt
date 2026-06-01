cmake_minimum_required(VERSION 3.10)
project(mymuduo CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB MYMUDUO_SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(echoserver
    examples/EchoServer.cpp
    ${MYMUDUO_SRC}
)

target_link_libraries(echoserver pthread)