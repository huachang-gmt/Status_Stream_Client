cmake_minimum_required(VERSION 3.16)

project(Status_Stream_Client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Status_Stream_Client
src/main.cpp
src/tcp_client.cpp
)

target_include_directories(Status_Stream_Client
PRIVATE
include
)

if (WIN32)
target_link_libraries(Status_Stream_Client
PRIVATE
ws2_32
)
endif()

add_executable(Packet_Assembly_Test
src/packet_assembly_test.cpp
src/tcp_client.cpp
)

target_include_directories(Packet_Assembly_Test
PRIVATE
include
)

if (WIN32)
target_link_libraries(Packet_Assembly_Test
PRIVATE
ws2_32
)

endif()