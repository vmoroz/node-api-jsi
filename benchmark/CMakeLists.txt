# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_definitions(-DNOMINMAX)

add_subdirectory(boost)
add_subdirectory(folly)

set(WITH_GFLAGS OFF)
set(WITH_GTEST OFF)
set(WITH_PKGCONFIG OFF)
set(WITH_SYMBOLIZE OFF)
set(WITH_THREADS OFF)
set(WITH_TLS OFF)
set(WITH_UNWIND OFF)

add_subdirectory(glog)

list(APPEND folly_sources
  ${FOLLY_SOURCE}/folly/Benchmark.cpp 
  ${FOLLY_SOURCE}/folly/concurrency/CacheLocality.cpp
  ${FOLLY_SOURCE}/folly/Conv.cpp
  ${FOLLY_SOURCE}/folly/Demangle.cpp
  ${FOLLY_SOURCE}/folly/detail/AsyncTrace.cpp
  ${FOLLY_SOURCE}/folly/detail/AtFork.cpp
  ${FOLLY_SOURCE}/folly/detail/Futex.cpp
  ${FOLLY_SOURCE}/folly/detail/MemoryIdler.cpp
  ${FOLLY_SOURCE}/folly/detail/StaticSingletonManager.cpp
  ${FOLLY_SOURCE}/folly/detail/ThreadLocalDetail.cpp
  ${FOLLY_SOURCE}/folly/detail/UniqueInstance.cpp
  ${FOLLY_SOURCE}/folly/ExceptionWrapper.cpp
  ${FOLLY_SOURCE}/folly/Executor.cpp
  ${FOLLY_SOURCE}/folly/executors/InlineExecutor.cpp
  ${FOLLY_SOURCE}/folly/executors/QueuedImmediateExecutor.cpp
  ${FOLLY_SOURCE}/folly/Format.cpp
  ${FOLLY_SOURCE}/folly/hash/SpookyHashV2.cpp
  ${FOLLY_SOURCE}/folly/io/async/Request.cpp
  ${FOLLY_SOURCE}/folly/lang/Assume.cpp
  ${FOLLY_SOURCE}/folly/lang/CString.cpp
  ${FOLLY_SOURCE}/folly/lang/SafeAssert.cpp
  ${FOLLY_SOURCE}/folly/memory/detail/MallocImpl.cpp
  ${FOLLY_SOURCE}/folly/memory/MallctlHelper.cpp
  ${FOLLY_SOURCE}/folly/portability/SysMembarrier.cpp
  ${FOLLY_SOURCE}/folly/ScopeGuard.cpp
  ${FOLLY_SOURCE}/folly/SharedMutex.cpp
  ${FOLLY_SOURCE}/folly/String.cpp
  ${FOLLY_SOURCE}/folly/synchronization/AsymmetricMemoryBarrier.cpp
  ${FOLLY_SOURCE}/folly/synchronization/Hazptr.cpp
  ${FOLLY_SOURCE}/folly/synchronization/ParkingLot.cpp
  ${FOLLY_SOURCE}/folly/synchronization/SanitizeThread.cpp
  ${FOLLY_SOURCE}/folly/dynamic.cpp
  ${FOLLY_SOURCE}/folly/json.cpp
  ${FOLLY_SOURCE}/folly/json_pointer.cpp
  ${FOLLY_SOURCE}/folly/Unicode.cpp
)

list(APPEND folly_sources
  ./stubs/windows_portability_tinyimp.cpp
  ./stubs/posix_stubs.cpp
)

add_executable(jsi_benchmarks
  "../jsi/jsi/decorator.h"
  "../jsi/jsi/instrumentation.h"
  "../jsi/jsi/jsi-inl.h"
  "../jsi/jsi/jsi.cpp"
  "../jsi/jsi/jsi.h"
  "../jsi/jsi/jsilib-posix.cpp"
  "../jsi/jsi/jsilib-windows.cpp"
  "../jsi/jsi/jsilib.h"
  "../jsi/jsi/threadsafe.h"
  "../src/ApiLoaders/HermesApi.cpp"
  "../src/ApiLoaders/HermesApi.h"
  "../src/ApiLoaders/JSRuntimeApi.cpp"
  "../src/ApiLoaders/JSRuntimeApi.h"
  "../src/ApiLoaders/NodeApi_win.cpp"
  "../src/ApiLoaders/NodeApi.cpp"
  "../src/ApiLoaders/NodeApi.h"
  "../src/NodeApiJsiRuntime.cpp"
  "../src/NodeApiJsiRuntime.h"
  "JSIMicrobenchmark.cpp"
  ${folly_sources}
)

target_include_directories(jsi_benchmarks PUBLIC
  ../jsi
  ../src
  ../node-api
  stubs
  ${BOOST_SOURCE}
  ${FOLLY_SOURCE}
  ${GLOG_SOURCE}
  ${GLOG_SOURCE}/src
  ${GLOG_BINARY})
target_link_libraries(jsi_benchmarks PUBLIC follybechmark)

include_directories(${BOOST_INCLUDES})
include_directories(${FOLLY_SOURCE})

add_definitions(-DFOLLY_MOBILE)
add_definitions(-DFOLLY_NO_CONFIG)
add_definitions(-DNOMINMAX)
add_definitions(-DFOLLY_EXPORT=)
add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

find_program(NUGET_EXE NAMES nuget)
if(NOT NUGET_EXE)
  message("NUGET.EXE not found.")
  message(FATAL_ERROR "Please install this executable, and run CMake again.")
endif()

execute_process(
  COMMAND ${NUGET_EXE}
    install "Microsoft.JavaScript.Hermes"
    -Version 0.1.14
    -ExcludeVersion
    -OutputDirectory ${CMAKE_BINARY_DIR}/packages
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(jsi_benchmarks PRIVATE ${CMAKE_BINARY_DIR}/packages/Microsoft.JavaScript.Hermes/build/native/Microsoft.JavaScript.Hermes.targets)
