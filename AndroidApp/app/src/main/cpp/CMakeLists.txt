cmake_minimum_required(VERSION 3.4.1)

# trick 'jni' required libs to think 'libz' are those
set(JAVA_AWT_LIBRARY z)
set(JAVA_JVM_LIBRARY z)
set(JAVA_INCLUDE_PATH2 z)
set(JAVA_AWT_INCLUDE_PATH z)

set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_LIST_DIR}/../java/cpp/shared)
add_subdirectory(CppSharedLogic)
