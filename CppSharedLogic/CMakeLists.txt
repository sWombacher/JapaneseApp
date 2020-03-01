cmake_minimum_required(VERSION 3.3)
project(CppSharedLogic LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

file(GLOB EXTERNAL_FILES
    3rd_party/external/*.h
    3rd_party/external/*.c
    3rd_party/external/*.cpp
)

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

find_package(Threads REQUIRED)
find_library(LibDl NAMES dl REQUIRED)

# we use boost since ndk r20 doesn't seem to support std::filesystem yet...
# boost for android can be build using this git repo:
# https://github.com/moritz-wundke/Boost-for-Android
find_package(Boost REQUIRED COMPONENTS filesystem)

set(ExternalLibs "external")
add_library(${ExternalLibs} STATIC ${EXTERNAL_FILES})
target_link_libraries(${ExternalLibs} PRIVATE Threads::Threads ${LibDl} ${Boost_LIBRARIES})

add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC include 3rd_party/external)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ExternalLibs})

option(BuildCliExecuteable "build cli executeable")
if (${BuildCliExecuteable})
    set(CLI_MODULE "cli")
    add_executable(${CLI_MODULE} "main.cpp")
    target_link_libraries(${CLI_MODULE} PRIVATE ${PROJECT_NAME})
endif()

find_package(SWIG REQUIRED)
find_package(JNI REQUIRED)
find_package(Java)
include(UseJava)
include(UseSWIG)

set(CMAKE_SWIG_FLAGS -package com.cpp.shared)

file(GLOB SWIG_FILES ./*.i)
set(SHARD_MODULE "shared")
include_directories(${JNI_INCLUDE_DIRS} include)
set_property(SOURCE ${SWIG_FILES} PROPERTY CPLUSPLUS ON)
swig_add_library(${SHARD_MODULE} LANGUAGE java SOURCES ${SWIG_FILES})
swig_link_libraries(${SHARD_MODULE} ${JNI_LIBRARIES} ${PROJECT_NAME})
