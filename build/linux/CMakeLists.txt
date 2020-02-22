project(TestAsyncExecutor)

cmake_minimum_required(VERSION 3.5)

ADD_DEFINITIONS(-g -Wall -pthread)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

INCLUDE_DIRECTORIES(../../src)

AUX_SOURCE_DIRECTORY(../../src SRC_DIR)
AUX_SOURCE_DIRECTORY(../../example SRC_DIR)

SET(EXECUTABLE_OUTPUT_PATH .)

ADD_EXECUTABLE(testAsync ${SRC_DIR})
TARGET_LINK_LIBRARIES(testAsync pthread rt)

