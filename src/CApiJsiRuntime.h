// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef CAPIJSIRUNTIME_H_
#define CAPIJSIRUNTIME_H_

#include <jsi/jsi.h>
#include <functional>
#include <memory>
#include "ApiLoaders/JSRuntimeApi.h"
#include <hermes/hermes_jsi.h>

namespace Microsoft::NodeApiJsi {

std::unique_ptr<facebook::jsi::Runtime>
makeCApiJsiRuntime(napi_env env, JSRuntimeApi *jsrApi, std::function<void()> onDelete) noexcept;

} // namespace Microsoft::NodeApiJsi

#endif // !CAPIJSIRUNTIME_H_
