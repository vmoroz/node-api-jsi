# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

include(FetchContent)

FetchContent_Declare(
  glog
  URL https://github.com/google/glog/archive/3a0d4d22c5ae0b9a2216988411cfa6bf860cc372.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_GetProperties(glog)

if(NOT glog_POPULATED)
  FetchContent_Populate(glog)

  # adds the targets: gtest, gtest_main, gmock, gmock_main
  add_subdirectory(
    ${glog_SOURCE_DIR}
    ${glog_BINARY_DIR}
    )
endif()

set(GLOG_SOURCE ${glog_SOURCE_DIR} PARENT_SCOPE)
set(GLOG_BINARY ${glog_BINARY_DIR} PARENT_SCOPE)
