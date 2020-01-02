cmake_minimum_required(VERSION 3.3)
project(CppSharedLogic LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs")
endif()

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

set(ExternalLibs "external")
add_library(${ExternalLibs} STATIC ${EXTERNAL_FILES})
target_link_libraries(${ExternalLibs} PRIVATE Threads::Threads ${LibDl})

add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC include 3rd_party/external)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ExternalLibs})

#set(CLI_MODULE "cli")
#add_executable(${CLI_MODULE} "main.cpp")
#target_link_libraries(${CLI_MODULE} PRIVATE ${PROJECT_NAME})

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
