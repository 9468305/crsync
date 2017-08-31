cmake_minimum_required (VERSION 3.8)
project (crsync C)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(crsync_VERSION_MAJOR 0)
set(crsync_VERSION_MINOR 3)
set(crsync_VERSION_PATCH 0)

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

#Put all binaries in same location
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

add_subdirectory(libs)
add_subdirectory(extra)
add_subdirectory(src)

add_definitions("-DHASH_BLOOM=21")
set(CMAKE_C_STANDARD 99)
# add_definitions("CURL_STATICLIB")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
include_directories(${CURL_HEADER_PATH})
include_directories(${EXTRA_HEADER_PATH})
# link_directories(${CMAKE_SOURCE_DIR}/lib)

find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_executable(crsync ${SOURCES} ${HEADERS} ${CURL_HEADERS} ${EXTRA_SOURCES} ${EXTRA_HEADERS})
target_link_libraries(
    crsync
    "${CURL_LIB}"
    "${OPENSSL_LIB}"
)

source_group("curl_inc" FILES ${CURL_HEADERS})
source_group("extra_inc" FILES ${EXTRA_HEADERS})
source_group("extra_src" FILES ${EXTRA_SOURCES})
