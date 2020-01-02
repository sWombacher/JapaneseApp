cmake_minimum_required(VERSION 3.1)
project(CppSharedLogic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

find_library(LibTinyXML NAMES tinyxml2)
find_library(LibSql NAMES sqlite3)

file(GLOB HEADER_FILES
    include/*.h
    include/*.hpp
    include/detail/*.h
    include/detail/*.hpp
)
file(GLOB SRC_FILES
    src/*.c
    src/*.cpp
    src/detail/*.c
    src/detail/*.cpp
)

add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LibTinyXML} ${LibSql})

set(CLI_MODULE "cli")
add_executable(${CLI_MODULE} "main.cpp")
target_link_libraries(${CLI_MODULE} PRIVATE ${PROJECT_NAME})

find_package(SWIG REQUIRED)
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)
include(UseSWIG)

file(GLOB SWIG_FILES ./*.i)
set(SHARD_MODULE "shared")
include_directories(${JNI_INCLUDE_DIRS} include)
set_property(SOURCE ${SWIG_FILES} PROPERTY CPLUSPLUS ON)
swig_add_library(${SHARD_MODULE} LANGUAGE java SOURCES ${SWIG_FILES})
swig_link_libraries(${SHARD_MODULE} ${JNI_LIBRARIES} ${PROJECT_NAME})
