cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(parasync LANGUAGES C CXX)

enable_language(C CXX ASM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -pg")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -pthread -mcrc32 -msse4.2")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(BuildType)
message (STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# Find POPT
find_package(POPT)
if (POPT_FOUND)
  message (STATUS "POPT_INCLUDE_DIRS  = ${POPT_INCLUDE_DIRS}")
  message (STATUS "POPT_LIBRARIES = ${POPT_LIBRARIES}")
  include_directories(${POPT_INCLUDE_DIRS})
endif (POPT_FOUND)

# Find ZLIB
find_package (ZLIB)
if (ZLIB_FOUND)
  message (STATUS "ZLIB_INCLUDE_DIRS  = ${ZLIB_INCLUDE_DIRS}")
  message (STATUS "ZLIB_LIBRARIES = ${ZLIB_LIBRARIES}")
  include_directories(${ZLIB_INCLUDE_DIRS})
endif (ZLIB_FOUND)

# Find libb2
find_package(LIBB2)
if (LIBB2_FOUND)
  message (STATUS "LIBB2_INCLUDE_DIRS  = ${LIBB2_INCLUDE_DIRS}")
  message (STATUS "LIBB2_LIBRARIES = ${LIBB2_LIBRARIES}")
endif (LIBB2_FOUND)

# Find photon
find_package(photon REQUIRED)
if (photon_FOUND)
  message (STATUS "photon_INCLUDE_DIRS  = ${photon_INCLUDE_DIRS}")
  message (STATUS "photon_LIBRARIES = ${photon_LIBRARIES}")
  include_directories(${photon_INCLUDE_DIRS})
endif (photon_FOUND)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_library(ISAL isal REQUIRED)
find_library(ISAL_crypto isal_crypto REQUIRED)
# find_library(IPSec_MB IPSec_MB REQUIRED)
find_library(PTHREAD pthread REQUIRED)
find_library(LIBRSYNC rsync REQUIRED)
find_package(mimalloc 2.1 REQUIRED)
find_library(LIBEV ev)
find_library(LIBEVENT event)
find_library(LIBEVENT2 event2)
find_library(LIBEVENT_OPENSSL event_openssl)
find_library(LIBLEVELDB leveldb)
find_library(LIBPROTOC protoc)

find_package(GFLAGS REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

# check where the library is installed
message(STATUS "ISAL = ${ISAL}")
message(STATUS "ISAL_crypto = ${ISAL_crypto}")
# message(STATUS "IPSec_MB = ${IPSec_MB}")
message(STATUS "LIBRSYNC = ${LIBRSYNC}")
message(STATUS "LIBEV = ${LIBEV}")
message(STATUS "LIBEVENT_OPENSSL = ${LIBEVENT_OPENSSL}")
message(STATUS "LIBEVENT = ${LIBEVENT}")
message(STATUS "LIBLEVELDB = ${LIBLEVELDB}")
message(STATUS "LIBPROTOC = ${LIBPROTOC}")

include_directories(thirdparty/brpc/output/include)
link_directories(thirdparty/brpc/output/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/crc32)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/parasync)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/b3-c)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/b2)
set(blake3_SOURCES)
list(APPEND blake3_SOURCES
    src/b3-c/blake3.c
    src/b3-c/blake3_dispatch.c
    src/b3-c/blake3_portable.c
    src/b3-c/blake3_sse2_x86-64_unix.S
    src/b3-c/blake3_sse41_x86-64_unix.S
    src/b3-c/blake3_avx2_x86-64_unix.S
    src/b3-c/blake3_avx512_x86-64_unix.S
    )
set(blake2_SOURCES)
list(APPEND blake2_SOURCES
    src/b2/blake2b.c
    src/b2/blake2bp.c
    )

# add_executable(rsync_test
#                tests/rsync/rsync_test.c
#               )
# target_link_libraries(rsync_test ${blake2_LIBS} ${POPT_LIBRARIES} ${LIBRSYNC})

# add_executable(skysync_f_test
#                tests/skysync-f/skysync_f_test.cpp
#                src/skysync_f.cpp
#                src/crc32/crc32.c
#                src/tools.cpp
#                src/fastcdc.cpp
#                src/ring_buffer.c
#                ${blake3_SOURCES}
#                ${blake2_SOURCES}
#                )
# target_link_libraries(skysync_f_test OpenSSL::Crypto ${ISAL})

# add_executable(skysync_c_test
#                tests/skysync-c/skysync_c_test.cpp
#                src/skysync_c.cpp
#                src/crc32/crc32.c
#                src/tools.cpp
#                src/fastcdc.cpp
#                src/skysync_f.cpp
#                src/ring_buffer.c
#                ${blake3_SOURCES}
#                ${blake2_SOURCES}
#                )
# target_link_libraries(skysync_c_test OpenSSL::Crypto ${ISAL} ${PTHREAD} mimalloc)

add_executable(dsync_test
               tests/dsync/dsync_test.cpp
               src/fastcdc.cpp
               src/crc32/crc32.c
               src/tools.cpp
               src/ring_buffer.c
               )
target_link_libraries(dsync_test ${ZLIB_LIBRARIES} OpenSSL::Crypto ${ISAL})

add_executable(para_sync_test
               tests/parasync/para_sync_test.cpp
               src/skysync_c.cpp
               src/crc32/crc32.c
               src/crc32/crc32c.c
               src/tools.cpp
               src/fastcdc.cpp
               src/skysync_f.cpp
               src/ring_buffer.c
               src/parallel_cdc.cpp
               src/parasync/chunker.cpp
               src/parasync/sync_worker.cpp
               ${blake3_SOURCES}
               ${blake2_SOURCES}
               )
target_include_directories(para_sync_test PUBLIC ${PHOTON_INCLUDE_DIR})
target_link_libraries(para_sync_test ${ZLIB_LIBRARIES} OpenSSL::Crypto ${ISAL} ${ISAL_crypto} ${PTHREAD} photon_static mimalloc)

add_executable(sync_task_test
               tests/parasync/sync_task_test.cpp
               src/skysync_c.cpp
               src/crc32/crc32.c
               src/crc32/crc32c.c
               src/tools.cpp
               src/fastcdc.cpp
               src/skysync_f.cpp
               src/ring_buffer.c
               src/parallel_cdc.cpp
               src/parasync/chunker.cpp
               src/parasync/sync_worker.cpp
               ${blake3_SOURCES}
               ${blake2_SOURCES}
               )
target_include_directories(sync_task_test PUBLIC ${PHOTON_INCLUDE_DIR})
target_link_libraries(sync_task_test ${ZLIB_LIBRARIES} OpenSSL::Crypto ${ISAL} ${ISAL_crypto} ${PTHREAD} photon_static mimalloc)

# add_executable(crc32c_test
#                tests/crc32c_test.cpp
#                src/crc32/crc32.c
#                src/tools.cpp
#                src/crc32/crc32c.c
#                )
# target_compile_options(crc32c_test PRIVATE -mcrc32 -msse4.2 -mssse3)
# target_link_libraries(crc32c_test ${ZLIB_LIBRARIES} ${ISAL} ${ISAL_crypto})

# add_executable(crc_rolling_test
#                tests/crc_rolling_test.c
#                )
# target_link_libraries(crc_rolling_test ${ZLIB_LIBRARIES})

# add_executable(crc32_rolling_test
#               tests/crc32_rolling_test.c
#               src/crc32/crc32.c
#               src/crc32/crc32c.c
#                )
# target_link_libraries(crc32_rolling_test ${ZLIB_LIBRARIES} ${ISAL})

add_executable(sha256_speed_test
               tests/sha256_speed_test.cpp
               ${blake3_SOURCES}
               ${blake2_SOURCES}
               )
target_compile_options(sha256_speed_test PRIVATE -msse4.1 -msha)
target_link_libraries(sha256_speed_test ${ZLIB_LIBRARIES} ${ISAL} ${ISAL_crypto} OpenSSL::SSL OpenSSL::Crypto)

add_executable(rpc_client_test
               src/net/rpc_client.cpp
               src/skysync_c.cpp
               src/crc32/crc32.c
               src/crc32/crc32c.c
               src/tools.cpp
               src/fastcdc.cpp
              #  src/skysync_f.cpp
               src/ring_buffer.c
              #  src/parallel_cdc.cpp
               src/parasync/chunker.cpp
               src/parasync/sync_worker.cpp
               src/net/echo.pb.cc
               ${blake3_SOURCES}
               )
target_link_libraries(rpc_client_test z m ${ISAL} ${ISAL_crypto} ${PTHREAD} photon_static mimalloc OpenSSL::SSL OpenSSL::Crypto ssl crypto gflags protobuf Threads::Threads ${OPENSSL_LIBRARIES} ${LIBPROTOC} brpc)

add_executable(rpc_server_test
               src/net/rpc_server.cpp
               src/skysync_c.cpp
               src/crc32/crc32.c
               src/crc32/crc32c.c
               src/tools.cpp
               src/fastcdc.cpp
              #  src/skysync_f.cpp
               src/ring_buffer.c
              #  src/parallel_cdc.cpp
               src/parasync/chunker.cpp
               src/parasync/sync_worker.cpp
               src/net/echo.pb.cc
               ${blake3_SOURCES}
               )
target_link_libraries(rpc_server_test z m ${ISAL} ${ISAL_crypto} ${PTHREAD} photon_static mimalloc OpenSSL::SSL OpenSSL::Crypto ssl crypto gflags protobuf Threads::Threads ${OPENSSL_LIBRARIES} ${LIBPROTOC} brpc)

add_executable(rpc_client_serial_test
               src/net/rpc_client_serial.cpp
               src/skysync_c.cpp
               src/crc32/crc32.c
               src/crc32/crc32c.c
               src/tools.cpp
               src/fastcdc.cpp
              #  src/skysync_f.cpp
               src/ring_buffer.c
              #  src/parallel_cdc.cpp
               src/parasync/chunker.cpp
               src/parasync/sync_worker.cpp
               src/net/echo.pb.cc
               ${blake3_SOURCES}
               )
target_link_libraries(rpc_client_serial_test z m ${ISAL} ${ISAL_crypto} ${PTHREAD} photon_static mimalloc OpenSSL::SSL OpenSSL::Crypto ssl crypto gflags protobuf Threads::Threads ${OPENSSL_LIBRARIES} ${LIBPROTOC} brpc)

add_executable(rpc_server_serial_test
               src/net/rpc_server_serial.cpp
               src/skysync_c.cpp
               src/crc32/crc32.c
               src/crc32/crc32c.c
               src/tools.cpp
               src/fastcdc.cpp
              #  src/skysync_f.cpp
               src/ring_buffer.c
              #  src/parallel_cdc.cpp
               src/parasync/chunker.cpp
               src/parasync/sync_worker.cpp
               src/net/echo.pb.cc
               ${blake3_SOURCES}
               )
target_link_libraries(rpc_server_serial_test z m ${ISAL} ${ISAL_crypto} ${PTHREAD} photon_static mimalloc OpenSSL::SSL OpenSSL::Crypto ssl crypto gflags protobuf Threads::Threads ${OPENSSL_LIBRARIES} ${LIBPROTOC} brpc)

# add_executable(dump_test
#                tests/rsyncx/dump_test.c
#                src/rsyncx.c
#                src/tools.c
#                ${rsync_LIB_SRCS}
#                )
# target_compile_options(dump_test PRIVATE -DLIBRSYNC_STATIC_DEFINE)
# # target_link_libraries(dump_test ${blake2_LIBS} ${POPT_LIBRARIES})
# # add_test(NAME dump_test COMMAND dump_test)

# add_executable(delta_test
#                tests/rsyncx/delta_test.c
#                src/rsyncx.c
#                src/tools.c
#                ${rsync_LIB_SRCS}
#                )
# target_compile_options(delta_test PRIVATE -DLIBRSYNC_STATIC_DEFINE -DHASH_BLOOM=27)

# add_executable(patch_test
#                tests/rsyncx/patch_test.c
#                src/rsyncx.c
#                src/tools.c
#                ${rsync_LIB_SRCS}
#                )
# target_compile_options(patch_test PRIVATE -DLIBRSYNC_STATIC_DEFINE)
# # add_test(NAME delta_test COMMAND delta_test)

# add_executable(http_post_delta
#                tests/http_post_delta.cpp
#                src/net.cpp
#                src/tools.cpp
#                )
# target_link_libraries(http_post_delta CURL::libcurl ${MHTD})

# add_executable(http_post_fastfp
#                tests/http_post_fastfp.cpp
#                src/net.cpp
#                src/tools.cpp
#                )
# target_link_libraries(http_post_fastfp CURL::libcurl ${MHTD})

# add_executable(http_receiver_test
#                tests/http_receiver_test.cpp
#                src/net/net.cpp
#                src/tools.cpp
#                src/net/mhd_server.cpp
#                )
# target_link_libraries(http_receiver_test CURL::libcurl ${MHTD})

# add_executable(fastfp_test
#                tests/dsync/fastfp_test.c
#                src/fastcdc.cpp
#                src/tools.c
#                src/ring_buffer.c
#                )
# # target_compile_options(fastcdc_test PRIVATE -DLIBRSYNC_STATIC_DEFINE)
# target_link_libraries(fastfp_test ${ZLIB_LIBRARIES} OpenSSL::Crypto)