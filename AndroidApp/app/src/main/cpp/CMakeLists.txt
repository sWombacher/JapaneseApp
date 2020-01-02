cmake_minimum_required(VERSION 3.4.1)

set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_INCLUDE_PATH2 NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)

set(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_LIST_DIR}/../java")

add_subdirectory(CppSharedLogic)

