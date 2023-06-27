/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

#include "hermes/CompileJS.h"
#include "hermes/hermes.h"

using namespace facebook::jsi;

static Function function(Runtime& rt, const char* code) {
  std::string bytecode;
  hermes::compileJS(code, bytecode, /* optimize = */ true);
  Value val = rt.evaluateJavaScript(
      std::unique_ptr<StringBuffer>(new StringBuffer(bytecode)), "");
  return val.getObject(rt).getFunction(rt);
}

BENCHMARK(CallJSFunc1, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  auto f = function(*rt, "(function (n) { return ++n; })");
  double result = 0.0;
  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    result = f.call(*rt, result).getNumber();
    folly::doNotOptimizeAway(result);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(CallJSFunc4, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  auto f = function(*rt, "(function (a, b, c, d) { return a + b + c + d; })");
  double result = 0.0;
  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    result = f.call(*rt, result, 1, 2, 3).getNumber();
    folly::doNotOptimizeAway(result);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK_RELATIVE(CallJSFunc4Fixed, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  auto f = function(*rt, "(function (a, b, c, d) { return a + b + c + d; })");
  double result = 0.0;
  Value args[4] = {Value(0), Value(1), Value(2), Value(3)};
  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    result = f.call(*rt, (const Value*)args, (size_t)4).getNumber();
    folly::doNotOptimizeAway(result);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(CallJSFunc1Object, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  auto obj = Object(*rt);
  obj.setProperty(*rt, "n", 1);
  Function f = function(*rt, "(function (obj) { return obj.n; })");
  double result = 0.0;
  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    Value args[] = {Value(*rt, obj)};
    double result =
        f.call(*rt, static_cast<const Value*>(args), static_cast<size_t>(1))
            .getNumber();
    folly::doNotOptimizeAway(result);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(MakeValueFromObject, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  auto obj = facebook::jsi::Object(*rt);
  obj.setProperty(*rt, "n", 1);
  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    auto val = Value(*rt, obj);
    folly::doNotOptimizeAway(val);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(CallHostFunc1, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  double result = 0.0;
  auto hf = Function::createFromHostFunction(
      *rt,
      PropNameID::forAscii(*rt, "hf"),
      0,
      [&result](Runtime&, const Value&, const Value* args, size_t) {
        result += args[0].getNumber();
        return Value(result);
      });
  auto f = function(*rt, "(function (hf, n) { while (n--) hf(n); })");

  // End of setup.
  braces.dismiss();

  f.call(*rt, hf, (double)n);
  folly::doNotOptimizeAway(result);

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(CallHostFunc4, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  double result = 0.0;
  auto hf = Function::createFromHostFunction(
      *rt,
      PropNameID::forAscii(*rt, "hf"),
      0,
      [&result](Runtime&, const Value&, const Value* args, size_t) {
        result += args[0].getNumber() + args[1].getNumber() +
            args[2].getNumber() + args[3].getNumber();
        return Value(result);
      });
  auto f = function(*rt, "(function (hf, n) { while (n--) hf(n, 1, 2, 3); })");

  // End of setup.
  braces.dismiss();

  f.call(*rt, hf, (double)n);
  folly::doNotOptimizeAway(result);

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(CreateHostObj, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  class EmptyHostObject : public HostObject {};
  auto ptr = std::make_shared<EmptyHostObject>();

  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    auto obj = Object::createFromHostObject(*rt, ptr);
    folly::doNotOptimizeAway(obj);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(GetJSProp, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  auto f = function(*rt, "(function () { return {foo: 42, bar: 87}; })");
  Object obj = f.call(*rt).getObject(*rt);
  auto foo = PropNameID::forAscii(*rt, "foo");

  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    auto result = obj.getProperty(*rt, foo);
    folly::doNotOptimizeAway(result);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(GetHostProp, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  class SimpleHostObject : public HostObject {
    Value get(Runtime&, const PropNameID&) override {
      return 42;
    }
  };
  auto ptr = std::make_shared<SimpleHostObject>();
  auto obj = Object::createFromHostObject(*rt, ptr);
  auto f = function(*rt, "(function (obj, n) { while (n--) obj.blah; })");

  // End of setup.
  braces.dismiss();

  f.call(*rt, obj, (double)n);

  // Exclude teardown.
  braces.rehire();
}

/// Access properties on a HostObject that keeps them in a JS object.
BENCHMARK(AccessHostObjectStateInJS, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  struct SimpleHostObject : public HostObject {
    Object jsObj;
    void* otherStuff;
    explicit SimpleHostObject(Object jsObj) : jsObj(std::move(jsObj)) {}
    Value get(Runtime& runtime, const PropNameID& propName) override {
      return jsObj.getProperty(runtime, propName);
    }
  };
  auto factory =
      function(*rt, "(function () { return {one: 1, two: 2, three: 3}; })");
  auto ptr =
      std::make_shared<SimpleHostObject>(factory.call(*rt).getObject(*rt));
  auto obj = Object::createFromHostObject(*rt, ptr);
  auto f = function(
      *rt,
      "(function (obj, n) { while (n--) obj.one + obj.two + obj.three; })");

  // End of setup.
  braces.dismiss();

  f.call(*rt, obj, (double)n);

  // Exclude teardown.
  braces.rehire();
}

/// Access properties on a HostObject that keeps them in C++ fields.
BENCHMARK(AccessHostObjectStateInNative, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  struct SimpleHostObject : public HostObject {
    int one{1};
    int two{2};
    int three{3};
    void* otherStuff;
    Value get(Runtime& runtime, const PropNameID& propName) override {
      auto name = propName.utf8(runtime);
      if (name == "one") {
        return one;
      } else if (name == "two") {
        return two;
      } else if (name == "three") {
        return three;
      }
      return Value::undefined();
    }
  };
  auto ptr = std::make_shared<SimpleHostObject>();
  auto obj = Object::createFromHostObject(*rt, ptr);
  auto f = function(
      *rt,
      "(function (obj, n) { while (n--) obj.one + obj.two + obj.three; })");

  // End of setup.
  braces.dismiss();

  f.call(*rt, obj, (double)n);

  // Exclude teardown.
  braces.rehire();
}

/// For comparison purposes, perform the same property accesses as in
/// AccessHostObjectStateIn{JS,Native} on a JS object with attached NativeState.
BENCHMARK(AccessNativeStateObj, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  struct SimpleNativeState : public NativeState {
    void* otherStuff;
  };
  auto factory =
      function(*rt, "(function () { return {one: 1, two: 2, three: 3}; })");
  auto obj = factory.call(*rt).getObject(*rt);
  obj.setNativeState(*rt, std::make_shared<SimpleNativeState>());

  auto f = function(
      *rt,
      "(function (obj, n) { while (n--) obj.one + obj.two + obj.three; })");

  // End of setup.
  braces.dismiss();

  f.call(*rt, obj, (double)n);

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(SetNativeState, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  struct SimpleNativeState : public NativeState {
    void* otherStuff;
  };
  auto ns = std::make_shared<SimpleNativeState>();
  auto factory =
      function(*rt, "(function () { return {one: 1, two: 2, three: 3}; })");
  auto obj = factory.call(*rt).getObject(*rt);

  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    // This will always allocate a new NativeState cell.
    obj.setNativeState(*rt, ns);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(GetNativeState, n) {
  // Must be first, to properly exclude setup and teardown.
  folly::BenchmarkSuspender braces;
  auto rt = facebook::hermes::makeHermesRuntime();
  struct SimpleNativeState : public NativeState {
    void* otherStuff;
  };
  auto ns = std::make_shared<SimpleNativeState>();
  auto factory =
      function(*rt, "(function () { return {one: 1, two: 2, three: 3}; })");
  auto obj = factory.call(*rt).getObject(*rt);
  obj.setNativeState(*rt, ns);

  // End of setup.
  braces.dismiss();

  for (int i = 0; i < n; ++i) {
    auto result = obj.getNativeState(*rt);
    folly::doNotOptimizeAway(result);
  }

  // Exclude teardown.
  braces.rehire();
}

BENCHMARK(ConstructAndDestructRuntime, n) {
  for (int i = 0; i < n; ++i) {
    auto rt = facebook::hermes::makeHermesRuntime();
    folly::doNotOptimizeAway(rt);
  }
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
