# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

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
  ../node-api)

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
