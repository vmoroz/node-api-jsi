# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# the following code to fetch googletest
# is inspired by and adapted after:
#   - https://cmake.org/cmake/help/v3.11/module/FetchContent.html
#
# We use version of GTest as in the https://github.com/Microsoft/TestAdapterForGoogleTest
# It allows to run GTests in Visual Studio and ADO.
# Please update the GIT_TAG below after the TAFGT is updated the reference.
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/06519cedc3159de8b36a504766ad6b7966555f10.zip
  DOWNLOAD_EXTRACT_TIMESTAMP
)

FetchContent_GetProperties(googletest)

if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)

  # Prevent GoogleTest from overriding our compiler/linker options
  # when building with Visual Studio
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  # Prevent GoogleTest from using PThreads
  set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)

  # adds the targets: gtest, gtest_main, gmock, gmock_main
  add_subdirectory(
    ${googletest_SOURCE_DIR}
    ${googletest_BINARY_DIR}
    )

  # Silence std::tr1 warning on MSVC
  if(MSVC)
    foreach(_tgt gtest gtest_main gmock gmock_main)
      target_compile_definitions(${_tgt}
        PRIVATE
          "_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING"
        )
    endforeach()
  endif()
endif()
