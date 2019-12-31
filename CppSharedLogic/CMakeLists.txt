cmake_minimum_required(VERSION 3.1)

project(CppSharedLogic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_executable(${PROJECT_NAME} "main.cpp")

target_sources(${PROJECT_NAME} PUBLIC ${HEADER_FILES} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LibTinyXML} ${LibSql})

# TODO add swig

